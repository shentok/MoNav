/*
 * Copyright 2010, 2011, 2012 mapsforge.org
 *
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MapDatabase.h"

#include "TileId.h"

#include <QDebug>
#include <QIODevice>
#include <QRect>

const QString MapDatabase::DEBUG_SIGNATURE_BLOCK = "block signature:";
const QString MapDatabase::DEBUG_SIGNATURE_POI = "POI signature:";
const QString MapDatabase::DEBUG_SIGNATURE_WAY = "way signature:";
const QString MapDatabase::INVALID_FIRST_WAY_OFFSET = "invalid first way offset:";
const QString MapDatabase::TAG_KEY_ELE = "ele";
const QString MapDatabase::TAG_KEY_HOUSE_NUMBER = "addr:housenumber";
const QString MapDatabase::TAG_KEY_NAME = "name";
const QString MapDatabase::TAG_KEY_REF = "ref";

MapDatabase::MapDatabase(QIODevice *inputFile) :
	m_inputFile(inputFile),
	m_databaseIndexCache(inputFile, INDEX_CACHE_SIZE),
	m_readBuffer(inputFile)
{
    Q_ASSERT(inputFile != 0);

    if (!inputFile->isOpen())
        return;

	m_readBuffer >> m_mapFileHeader;
}

MapDatabase::~MapDatabase()
{
}

MapFileInfo MapDatabase::getMapFileInfo() const
{
	return m_mapFileHeader.getMapFileInfo();
}

VectorTile MapDatabase::readMapData(const TileId &tile)
{
	static const qint64 BITMASK_INDEX_OFFSET = 0x7FFFFFFFFFLL;
	static const qint64 BITMASK_INDEX_WATER = 0x8000000000LL;

	const BoundingBox bbox = m_mapFileHeader.getMapFileInfo().boundingBox();
	const byte zoomLevel = m_mapFileHeader.getQueryZoomLevel(tile.zoomLevel);

	// get and check the sub-file for the query zoom level
	const SubFileParameter subFileParameter = m_mapFileHeader.getSubFileParameter(zoomLevel);
	if (subFileParameter.isNull()) {
		qWarning() << "no sub-file for zoom level:" << zoomLevel;
		return VectorTile();
	}

	const QRect physicalRect(QPoint(MercatorProjection::boundaryTileLeft(bbox, subFileParameter.physicalZoomLevel()),
									MercatorProjection::boundaryTileTop(bbox, subFileParameter.physicalZoomLevel())),
							 QPoint(MercatorProjection::boundaryTileRight(bbox, subFileParameter.physicalZoomLevel()),
									MercatorProjection::boundaryTileBottom(bbox, subFileParameter.physicalZoomLevel())));
	const quint64 numberOfBlocks = physicalRect.width() * physicalRect.height();
	const QueryParameters queryParameters = calculateBlocks(tile, physicalRect, zoomLevel, subFileParameter.physicalZoomLevel());

	QList<PointOfInterest> pointsOfInterest;
	QList<Way> ways;
	bool queryIsWater = true;
	bool queryReadWaterInfo = false;

	// read and process all blocks from top to bottom and from left to right
	for (qint64 row = queryParameters.fromBlockY; row < queryParameters.toBlockY; ++row) {
		for (qint64 column = queryParameters.fromBlockX; column < queryParameters.toBlockX; ++column) {
			// calculate the actual block number of the needed block in the file
			const quint64 blockNumber = row * physicalRect.width() + column;

			// get the current index entry
			const quint64 currentBlockIndexEntry = m_databaseIndexCache.getIndexEntry(subFileParameter, blockNumber, numberOfBlocks);

			// check if the current query would still return a water tile
			if (queryIsWater) {
				// check the water flag of the current block in its index entry
				queryIsWater &= (currentBlockIndexEntry & BITMASK_INDEX_WATER) != 0;
				queryReadWaterInfo = true;
			}

			// get and check the current block pointer
			const qint64 currentBlockPointer = currentBlockIndexEntry & BITMASK_INDEX_OFFSET;
			if (currentBlockPointer < 1 || currentBlockPointer > subFileParameter.subFileSize()) {
				qWarning() << "invalid current block pointer:" << currentBlockPointer;
				qWarning() << "subFileSize:" << subFileParameter.subFileSize();
				continue;
			}

			qint64 nextBlockPointer;
			// check if the current block is the last block in the file
			if (blockNumber + 1 == numberOfBlocks) {
				// set the next block pointer to the end of the file
				nextBlockPointer = subFileParameter.subFileSize();
			} else {
				// get and check the next block pointer
				nextBlockPointer = m_databaseIndexCache.getIndexEntry(subFileParameter, blockNumber + 1, numberOfBlocks) & BITMASK_INDEX_OFFSET;
				if (nextBlockPointer > subFileParameter.subFileSize()) {
					qWarning() << "invalid next block pointer:" << nextBlockPointer;
					qWarning() << "sub-file size:" << subFileParameter.subFileSize();
					continue;
				}
			}

			// calculate the size of the current block
			const int currentBlockSize = (int) (nextBlockPointer - currentBlockPointer);
			if (currentBlockSize < 0) {
				qWarning() << "current block size must not be negative:" << currentBlockSize;
				continue;
			} else if (currentBlockSize == 0) {
				// the current block is empty, continue with the next block
				continue;
			} else if (currentBlockSize > ReadBuffer::MAXIMUM_BUFFER_SIZE) {
				// the current block is too large, continue with the next block
				qWarning() << "current block size too large:" << currentBlockSize;
				continue;
			} else if (currentBlockPointer + currentBlockSize > m_inputFile->size()) {
				qWarning() << "current block largher than file size:" << currentBlockSize;
				continue;
			}

			// seek to the current block in the map file
			m_inputFile->seek(subFileParameter.startAddress() + currentBlockPointer);

			// read the current block into the buffer
			if (!m_readBuffer.readFromFile(currentBlockSize)) {
				// skip the current block
				qWarning() << "reading current block has failed:" << currentBlockSize;
				continue;
			}

			// calculate the top-left coordinates of the underlying tile
			const TileId subTile = TileId(physicalRect.left() + column,
									  physicalRect.top() + row,
									  subFileParameter.physicalZoomLevel());
			const LatLong tileCoordinates = MercatorProjection::tileToCoordinates(subTile);

			const VectorTile vectorTile = readVectorTile(queryParameters, subFileParameter, tileCoordinates);
			pointsOfInterest << vectorTile.pointsOfInterest();
			ways << vectorTile.ways();
		}
	}

	// the query is finished, was the water flag set for all blocks?
	const bool isWater = queryIsWater && queryReadWaterInfo;

	return VectorTile(pointsOfInterest, ways, isWater);
}

QueryParameters MapDatabase::calculateBlocks(const TileId &tile, const QRect &physicalRect, int zoomLevel, int physicalZoomLevel)
{
	qint64 fromPhysicalTileX;
	qint64 fromPhysicalTileY;
	qint64 toPhysicalTileX;
	qint64 toPhysicalTileY;
	bool useTileBitmask = false;

	if (tile.zoomLevel <= physicalZoomLevel) {
		// calculate the XY numbers of the upper left and lower right sub-tiles
		const int zoomLevelDifference = physicalZoomLevel - tile.zoomLevel;
		fromPhysicalTileX = tile.tileX << zoomLevelDifference;
		fromPhysicalTileY = tile.tileY << zoomLevelDifference;
		toPhysicalTileX = fromPhysicalTileX + (1 << zoomLevelDifference) - 1;
		toPhysicalTileY = fromPhysicalTileY + (1 << zoomLevelDifference) - 1;
	} else {
		// calculate the XY numbers of the parent base tile
		const int zoomLevelDifference = tile.zoomLevel - physicalZoomLevel;
		fromPhysicalTileX = tile.tileX >> zoomLevelDifference;
		fromPhysicalTileY = tile.tileY >> zoomLevelDifference;
		toPhysicalTileX = fromPhysicalTileX;
		toPhysicalTileY = fromPhysicalTileY;
		useTileBitmask = true;
	}

	// calculate the blocks in the file which need to be read
	const qint64 fromBlockX = qMax<qint64>(fromPhysicalTileX - physicalRect.left(), 0L);
	const qint64 fromBlockY = qMax<qint64>(fromPhysicalTileY - physicalRect.top(), 0L);
	const qint64 toBlockX = qMin<qint64>(toPhysicalTileX, physicalRect.right()) - physicalRect.left() + 1;
	const qint64 toBlockY = qMin<qint64>(toPhysicalTileY, physicalRect.bottom()) - physicalRect.top() + 1;

	if (useTileBitmask) {
		return QueryParameters(zoomLevel, fromBlockX, fromBlockY, toBlockX, toBlockY, tile, tile.zoomLevel - physicalZoomLevel);
	}

	return QueryParameters(zoomLevel, fromBlockX, fromBlockY, toBlockX, toBlockY);
}

void MapDatabase::decodeWayNodesDoubleDelta(QVector<GeoPoint> &waySegment, const LatLong &tileCoordinates)
{
	// get the first way node latitude offset (VBE-S)
	double wayNodeLatitude = tileCoordinates.lat()
			+ LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

	// get the first way node longitude offset (VBE-S)
	double wayNodeLongitude = tileCoordinates.lon()
			+ LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

	// store the first way node
	waySegment[0] = GeoPoint(wayNodeLatitude, wayNodeLongitude);

	double previousSingleDeltaLatitude = 0;
	double previousSingleDeltaLongitude = 0;

	for (int wayNodesIndex = 1; wayNodesIndex < waySegment.size(); ++wayNodesIndex) {
		// get the way node latitude double-delta offset (VBE-S)
		double doubleDeltaLatitude = LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

		// get the way node longitude double-delta offset (VBE-S)
		double doubleDeltaLongitude = LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

		double singleDeltaLatitude = doubleDeltaLatitude + previousSingleDeltaLatitude;
		double singleDeltaLongitude = doubleDeltaLongitude + previousSingleDeltaLongitude;

		wayNodeLatitude = wayNodeLatitude + singleDeltaLatitude;
		wayNodeLongitude = wayNodeLongitude + singleDeltaLongitude;

		waySegment[wayNodesIndex] = GeoPoint(wayNodeLatitude, wayNodeLongitude);

		previousSingleDeltaLatitude = singleDeltaLatitude;
		previousSingleDeltaLongitude = singleDeltaLongitude;
	}
}

void MapDatabase::decodeWayNodesSingleDelta(QVector<GeoPoint> &waySegment, const LatLong &tileCoordinates)
{
	// get the first way node latitude single-delta offset (VBE-S)
	double wayNodeLatitude = tileCoordinates.lat()
			+ LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

	// get the first way node longitude single-delta offset (VBE-S)
	double wayNodeLongitude = tileCoordinates.lon()
			+ LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

	// store the first way node
	waySegment[0] = LatLong(wayNodeLatitude, wayNodeLongitude);

	for (int wayNodesIndex = 1; wayNodesIndex < waySegment.size(); ++wayNodesIndex) {
		// get the way node latitude offset (VBE-S)
		wayNodeLatitude = wayNodeLatitude + LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

		// get the way node longitude offset (VBE-S)
		wayNodeLongitude = wayNodeLongitude
				+ LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

		waySegment[wayNodesIndex] = LatLong(wayNodeLatitude, wayNodeLongitude);
	}
}

void MapDatabase::logDebugSignatures()
{
	if (m_mapFileHeader.getMapFileInfo().debugFile()) {
		qWarning() << DEBUG_SIGNATURE_WAY << m_signatureWay;
		qWarning() << DEBUG_SIGNATURE_BLOCK << m_signatureBlock;
	}
}

VectorTile MapDatabase::readVectorTile(const QueryParameters &queryParameters, const SubFileParameter &subFileParameter, const LatLong &tileCoordinates)
{
	if (m_mapFileHeader.getMapFileInfo().debugFile()) {
		// get and check the block signature
		m_signatureBlock = m_readBuffer.readUTF8EncodedString(SIGNATURE_LENGTH_BLOCK);
		if (!m_signatureBlock.startsWith("###TileStart")) {
			qWarning() << "invalid block signature:" << m_signatureBlock;
			return VectorTile();
		}
	}

	QVector<Foo> zoomTable = readZoomTable(subFileParameter);
	if (zoomTable.isEmpty()) {
		return VectorTile();
	}

	const int zoomTableRow = queryParameters.queryZoomLevel - subFileParameter.zoomLevelMin();
	const int poisOnQueryZoomLevel = zoomTable[zoomTableRow].numberOfPois;
	const int waysOnQueryZoomLevel = zoomTable[zoomTableRow].numberOfWays;

	// get the relative offset to the first stored way in the block
	int firstWayOffset = m_readBuffer.readUnsignedInt();
	if (firstWayOffset < 0) {
		qWarning() << INVALID_FIRST_WAY_OFFSET << firstWayOffset;
		if (m_mapFileHeader.getMapFileInfo().debugFile()) {
			qWarning() << DEBUG_SIGNATURE_BLOCK << m_signatureBlock;
		}
		return VectorTile();
	}

	// add the current buffer position to the relative first way offset
	firstWayOffset += m_readBuffer.getBufferPosition();
	if (firstWayOffset > m_readBuffer.getBufferSize()) {
		qWarning() << INVALID_FIRST_WAY_OFFSET << firstWayOffset;
		if (m_mapFileHeader.getMapFileInfo().debugFile()) {
			qWarning() << DEBUG_SIGNATURE_BLOCK << m_signatureBlock;
		}
		return VectorTile();
	}

	const QList<PointOfInterest> pois = processPOIs(poisOnQueryZoomLevel, tileCoordinates);

	// finished reading POIs, check if the current buffer position is valid
	if (m_readBuffer.getBufferPosition() > firstWayOffset) {
		qWarning() << "invalid buffer position:" << m_readBuffer.getBufferPosition();
		if (m_mapFileHeader.getMapFileInfo().debugFile()) {
			qWarning() << DEBUG_SIGNATURE_BLOCK << m_signatureBlock;
		}
		return VectorTile();
	}

	// move the pointer to the first way
	m_readBuffer.setBufferPosition(firstWayOffset);

	const QList<Way> ways = processWays(queryParameters, waysOnQueryZoomLevel, tileCoordinates);

	return VectorTile(pois, ways, false);
}

QList<PointOfInterest> MapDatabase::processPOIs(int numberOfPois, const LatLong &tileCoordinates)
{
	QList<PointOfInterest> pois;
	QVector<Tag> poiTags = m_mapFileHeader.getMapFileInfo().poiTags();

	for (int elementCounter = numberOfPois; elementCounter != 0; --elementCounter) {
		if (m_mapFileHeader.getMapFileInfo().debugFile()) {
			// get and check the POI signature
			m_signaturePoi = m_readBuffer.readUTF8EncodedString(SIGNATURE_LENGTH_POI);
			if (!m_signaturePoi.startsWith("***POIStart")) {
				qWarning() << "invalid POI signature:" << m_signaturePoi;
				qWarning() << DEBUG_SIGNATURE_BLOCK << m_signatureBlock;
				return QList<PointOfInterest>();
			}
		}

		// get the POI latitude offset (VBE-S)
		double latitude = tileCoordinates.lat()
				+ LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

		// get the POI longitude offset (VBE-S)
		double longitude = tileCoordinates.lon()
				+ LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

		// get the special byte which encodes multiple flags
		byte specialByte = m_readBuffer.readByte();

		// bit 1-4 represent the layer
		byte layer = (byte) ((specialByte & POI_LAYER_BITMASK) >> POI_LAYER_SHIFT);
		// bit 5-8 represent the number of tag IDs
		byte numberOfTags = (byte) (specialByte & POI_NUMBER_OF_TAGS_BITMASK);

		QList<Tag> tags;

		// get the tag IDs (VBE-U)
		for (byte tagIndex = numberOfTags; tagIndex != 0; --tagIndex) {
			int tagId = m_readBuffer.readUnsignedInt();
			if (tagId < 0 || tagId >= poiTags.size()) {
				qWarning() <<"invalid POI tag ID:" << tagId;
				if (m_mapFileHeader.getMapFileInfo().debugFile()) {
					qWarning() << DEBUG_SIGNATURE_POI << m_signaturePoi;
					qWarning() << DEBUG_SIGNATURE_BLOCK << m_signatureBlock;
				}
				return QList<PointOfInterest>();
			}
			tags.append(poiTags[tagId]);
		}

		// get the feature bitmask (1 byte)
		byte featureByte = m_readBuffer.readByte();

		// bit 1-3 enable optional features
		bool featureName = (featureByte & POI_FEATURE_NAME) != 0;
		bool featureHouseNumber = (featureByte & POI_FEATURE_HOUSE_NUMBER) != 0;
		bool featureElevation = (featureByte & POI_FEATURE_ELEVATION) != 0;

		// check if the POI has a name
		if (featureName) {
			tags.append(Tag(TAG_KEY_NAME, m_readBuffer.readUTF8EncodedString()));
		}

		// check if the POI has a house number
		if (featureHouseNumber) {
			tags.append(Tag(TAG_KEY_HOUSE_NUMBER, m_readBuffer.readUTF8EncodedString()));
		}

		// check if the POI has an elevation
		if (featureElevation) {
			tags.append(Tag(TAG_KEY_ELE, QString::number(m_readBuffer.readSignedInt())));
		}

		pois.append(PointOfInterest(layer, tags, LatLong(latitude, longitude)));
	}

	return pois;
}

QVector<QVector<GeoPoint> > MapDatabase::processWayDataBlock(bool doubleDeltaEncoding, const LatLong &tileCoordinates)
{
	// get and check the number of way coordinate blocks (VBE-U)
	int numberOfWayCoordinateBlocks = m_readBuffer.readUnsignedInt();
	if (numberOfWayCoordinateBlocks < 1 || numberOfWayCoordinateBlocks > std::numeric_limits<short>::max()) {
		qWarning() << "invalid number of way coordinate blocks:" << numberOfWayCoordinateBlocks;
		logDebugSignatures();
		return QVector< QVector<GeoPoint> >();
	}

	// create the array which will store the different way coordinate blocks
	QVector< QVector<LatLong> > wayCoordinates(numberOfWayCoordinateBlocks);

	// read the way coordinate blocks
	for (int coordinateBlock = 0; coordinateBlock < numberOfWayCoordinateBlocks; ++coordinateBlock) {
		// get and check the number of way nodes (VBE-U)
		int numberOfWayNodes = m_readBuffer.readUnsignedInt();
		if (numberOfWayNodes < 2 || numberOfWayNodes > MAXIMUM_WAY_NODES_SEQUENCE_LENGTH) {
			qWarning() << "invalid number of way nodes:" << numberOfWayNodes;
			logDebugSignatures();
			return QVector< QVector<GeoPoint> >();
		}

		// create the array which will store the current way segment
		QVector<LatLong> waySegment(numberOfWayNodes);

		if (doubleDeltaEncoding) {
			decodeWayNodesDoubleDelta(waySegment, tileCoordinates);
		} else {
			decodeWayNodesSingleDelta(waySegment, tileCoordinates);
		}

		wayCoordinates[coordinateBlock] = waySegment;
	}

	return wayCoordinates;
}

QList<Way> MapDatabase::processWays(const QueryParameters &queryParameters, int numberOfWays, const LatLong &tileCoordinates)
{
	QList<Way> ways;
	QVector<Tag> wayTags = m_mapFileHeader.getMapFileInfo().wayTags();

	for (int elementCounter = numberOfWays; elementCounter != 0; --elementCounter) {
		if (m_mapFileHeader.getMapFileInfo().debugFile()) {
			// get and check the way signature
			m_signatureWay = m_readBuffer.readUTF8EncodedString(SIGNATURE_LENGTH_WAY);
			if (!m_signatureWay.startsWith("---WayStart")) {
				qWarning() << "invalid way signature:" << m_signatureWay;
				qWarning() << DEBUG_SIGNATURE_BLOCK << m_signatureBlock;
				return QList<Way>();
			}
		}

		// get the size of the way (VBE-U)
		int wayDataSize = m_readBuffer.readUnsignedInt();
		if (wayDataSize < 0) {
			qWarning() << "invalid way data size:" << wayDataSize;
			if (m_mapFileHeader.getMapFileInfo().debugFile()) {
				qWarning() << DEBUG_SIGNATURE_BLOCK << m_signatureBlock;
			}
			return QList<Way>();
		}

		if (queryParameters.useTileBitmask) {
			// get the way tile bitmask (2 bytes)
			int tileBitmask = m_readBuffer.readShort();
			// check if the way is inside the requested tile
			if ((queryParameters.queryTileBitmask & tileBitmask) == 0) {
				// skip the rest of the way and continue with the next way
				m_readBuffer.skipBytes(wayDataSize - 2);
				continue;
			}
		} else {
			// ignore the way tile bitmask (2 bytes)
			m_readBuffer.skipBytes(2);
		}

		// get the special byte which encodes multiple flags
		byte specialByte = m_readBuffer.readByte();

		// bit 1-4 represent the layer
		byte layer = (byte) ((specialByte & WAY_LAYER_BITMASK) >> WAY_LAYER_SHIFT);
		// bit 5-8 represent the number of tag IDs
		byte numberOfTags = (byte) (specialByte & WAY_NUMBER_OF_TAGS_BITMASK);

		QList<Tag> tags;

		for (byte tagIndex = numberOfTags; tagIndex != 0; --tagIndex) {
			int tagId = m_readBuffer.readUnsignedInt();
			if (tagId < 0 || tagId >= wayTags.size()) {
				qWarning() << "invalid way tag ID:" << tagId;
				logDebugSignatures();
				return QList<Way>();
			}
			tags.append(wayTags[tagId]);
		}

		// get the feature bitmask (1 byte)
		byte featureByte = m_readBuffer.readByte();

		// bit 1-6 enable optional features
		bool featureName = (featureByte & WAY_FEATURE_NAME) != 0;
		bool featureHouseNumber = (featureByte & WAY_FEATURE_HOUSE_NUMBER) != 0;
		bool featureRef = (featureByte & WAY_FEATURE_REF) != 0;
		bool featureLabelPosition = (featureByte & WAY_FEATURE_LABEL_POSITION) != 0;
		bool featureWayDataBlocksByte = (featureByte & WAY_FEATURE_DATA_BLOCKS_BYTE) != 0;
		bool featureWayDoubleDeltaEncoding = (featureByte & WAY_FEATURE_DOUBLE_DELTA_ENCODING) != 0;

		// check if the way has a name
		if (featureName) {
			tags.append(Tag(TAG_KEY_NAME, m_readBuffer.readUTF8EncodedString()));
		}

		// check if the way has a house number
		if (featureHouseNumber) {
			tags.append(Tag(TAG_KEY_HOUSE_NUMBER, m_readBuffer.readUTF8EncodedString()));
		}

		// check if the way has a reference
		if (featureRef) {
			tags.append(Tag(TAG_KEY_REF, m_readBuffer.readUTF8EncodedString()));
		}

		LatLong labelPosition = readOptionalLabelPosition(featureLabelPosition, tileCoordinates);

		int wayDataBlocks = readOptionalWayDataBlocksByte(featureWayDataBlocksByte);
		if (wayDataBlocks < 1) {
			qWarning() << "invalid number of way data blocks:" << wayDataBlocks;
			logDebugSignatures();
			return QList<Way>();
		}

		for (int wayDataBlock = 0; wayDataBlock < wayDataBlocks; ++wayDataBlock) {
			const QVector< QVector<GeoPoint> > wayNodes = processWayDataBlock(featureWayDoubleDeltaEncoding, tileCoordinates);
			if (wayNodes.isEmpty()) {
				return QList<Way>();
			}

			ways.append(Way(layer, tags, wayNodes, labelPosition));
		}
	}

	return ways;
}

LatLong MapDatabase::readOptionalLabelPosition(bool featureLabelPosition, const LatLong &tileCoordinates)
{
	if (featureLabelPosition) {
		// get the label position latitude offset (VBE-S)
		double latitude = tileCoordinates.lat()
				+ LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

		// get the label position longitude offset (VBE-S)
		double longitude = tileCoordinates.lon()
				+ LatLong::microdegreesToDegrees(m_readBuffer.readSignedInt());

		return LatLong(latitude, longitude);
	}

	return LatLong();
}

int MapDatabase::readOptionalWayDataBlocksByte(bool featureWayDataBlocksByte)
{
	if (featureWayDataBlocksByte) {
		// get and check the number of way data blocks (VBE-U)
		return m_readBuffer.readUnsignedInt();
	}
	// only one way data block exists
	return 1;
}

QVector<MapDatabase::Foo> MapDatabase::readZoomTable(const SubFileParameter &subFileParameter)
{
	const int rows = subFileParameter.zoomLevelMax() - subFileParameter.zoomLevelMin() + 1;
	QVector<Foo> zoomTable(rows);

	unsigned int cumulatedNumberOfPois = 0;
	unsigned int cumulatedNumberOfWays = 0;

	for (int row = 0; row < rows; ++row) {
		cumulatedNumberOfPois += m_readBuffer.readUnsignedInt();
		cumulatedNumberOfWays += m_readBuffer.readUnsignedInt();

		zoomTable[row].numberOfPois = cumulatedNumberOfPois;
		zoomTable[row].numberOfWays = cumulatedNumberOfWays;
	}

	return zoomTable;
}

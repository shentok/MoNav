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

#ifndef MAPSFORGE_MAPDATABASE_H
#define MAPSFORGE_MAPDATABASE_H

#include "header/MapFileHeader.h"
#include "header/MapFileInfo.h"
#include "MercatorProjection.h"
#include "QueryParameters.h"
#include "IndexCache.h"
#include "VectorTile.h"

#include <QString>
#include <QVector>

class QIODevice;

/**
 * A class for reading binary map files.
 * <p>
 * This class is not thread-safe. Each thread should use its own instance.
 * 
 * @see <a href="https://code.google.com/p/mapsforge/wiki/SpecificationBinaryMapFile">Specification</a>
 */
class MapDatabase
{
public:
	struct Foo {
		unsigned int numberOfPois;
		unsigned int numberOfWays;
	};

	class TileIdRange {
	public:
		TileIdRange(qint64 left, qint64 top, qint64 right, qint64 bottom, int zoomLevel);

		qint64 left() const;
		qint64 top() const;
		qint64 right() const;
		qint64 bottom() const;
		int zoomLevel() const;

		qint64 width() const;
		qint64 height() const;

	private:
		qint64 m_left;
		qint64 m_top;
		qint64 m_right;
		qint64 m_bottom;
		int m_zoomLevel;
	};

	/**
	 * Opens the given map file, reads its header data and validates them.
	 *
	 * @param mapFile
	 *            the map file.
	 * @return a FileOpenResult containing an error message in case of a failure.
	 * @throws IllegalArgumentException
	 *             if the given map file is null.
	 */
	MapDatabase(QIODevice *inputFile);

	/**
	 * Closes the map file and destroys all internal caches. Has no effect if no map file is currently opened.
	 */
	~MapDatabase();

	/**
	 * @return the metadata for the current map file.
	 * @throws IllegalStateException
	 *             if no map is currently opened.
	 */
	MapFileInfo getMapFileInfo() const;

	/**
	 * Reads all map data for the area covered by the given tile at the tile zoom level.
	 * 
	 * @param tile
	 *            defines area and zoom level of read map data.
	 * @return the read map data.
	 */
	VectorTile readMapData(const TileId &tile);

private:
	static TileIdRange physicalMapRect2TileRect(const TileId &tile, const TileIdRange &mapRect);
	static QueryParameters calculateBlocks(const TileId &tile, int zoomLevel, int physicalZoomLevel);

	void decodeWayNodesDoubleDelta(QVector<GeoPoint> &waySegment, const LatLong &tileCoordinates);

	void decodeWayNodesSingleDelta(QVector<GeoPoint> &waySegment, const LatLong &tileCoordinates);

	/**
	 * Logs the debug signatures of the current way and block.
	 */
	void logDebugSignatures();

	VectorTile readVectorTile(const QueryParameters &queryParameters, const SubFileParameter &subFileParameter, const LatLong &tileCoordinates);

	QList<PointOfInterest> processPOIs(int numberOfPois, const LatLong &tileCoordinates);

	QVector< QVector<GeoPoint> > processWayDataBlock(bool doubleDeltaEncoding, const LatLong &tileCoordinates);

	QList<Way> processWays(const QueryParameters &queryParameters, int numberOfWays, const LatLong &tileCoordinates);

	LatLong readOptionalLabelPosition(bool featureLabelPosition, const LatLong &tileCoordinates);

	int readOptionalWayDataBlocksByte(bool featureWayDataBlocksByte);

	QVector<Foo> readZoomTable(const SubFileParameter &subFileParameter);

private:
	/**
	 * Debug message prefix for the block signature.
	 */
	static const QString DEBUG_SIGNATURE_BLOCK;

	/**
	 * Debug message prefix for the POI signature.
	 */
	static const QString DEBUG_SIGNATURE_POI;

	/**
	 * Debug message prefix for the way signature.
	 */
	static const QString DEBUG_SIGNATURE_WAY;

	/**
	 * Amount of cache blocks that the index cache should store.
	 */
	static const int INDEX_CACHE_SIZE = 64;

	/**
	 * Error message for an invalid first way offset.
	 */
	static const QString INVALID_FIRST_WAY_OFFSET;

	/**
	 * Maximum way nodes sequence length which is considered as valid.
	 */
	static const int MAXIMUM_WAY_NODES_SEQUENCE_LENGTH = 8192;

	/**
	 * Bitmask for the optional POI feature "elevation".
	 */
	static const int POI_FEATURE_ELEVATION = 0x20;

	/**
	 * Bitmask for the optional POI feature "house number".
	 */
	static const int POI_FEATURE_HOUSE_NUMBER = 0x40;

	/**
	 * Bitmask for the optional POI feature "name".
	 */
	static const int POI_FEATURE_NAME = 0x80;

	/**
	 * Bitmask for the POI layer.
	 */
	static const int POI_LAYER_BITMASK = 0xf0;

	/**
	 * Bit shift for calculating the POI layer.
	 */
	static const int POI_LAYER_SHIFT = 4;

	/**
	 * Bitmask for the number of POI tags.
	 */
	static const int POI_NUMBER_OF_TAGS_BITMASK = 0x0f;

	/**
	 * Length of the debug signature at the beginning of each block.
	 */
	static const byte SIGNATURE_LENGTH_BLOCK = 32;

	/**
	 * Length of the debug signature at the beginning of each POI.
	 */
	static const byte SIGNATURE_LENGTH_POI = 32;

	/**
	 * Length of the debug signature at the beginning of each way.
	 */
	static const byte SIGNATURE_LENGTH_WAY = 32;

	/**
	 * The key of the elevation OpenStreetMap tag.
	 */
	static const QString TAG_KEY_ELE;

	/**
	 * The key of the house number OpenStreetMap tag.
	 */
	static const QString TAG_KEY_HOUSE_NUMBER;

	/**
	 * The key of the name OpenStreetMap tag.
	 */
	static const QString TAG_KEY_NAME;

	/**
	 * The key of the reference OpenStreetMap tag.
	 */
	static const QString TAG_KEY_REF;

	/**
	 * Bitmask for the optional way data blocks byte.
	 */
	static const int WAY_FEATURE_DATA_BLOCKS_BYTE = 0x08;

	/**
	 * Bitmask for the optional way double delta encoding.
	 */
	static const int WAY_FEATURE_DOUBLE_DELTA_ENCODING = 0x04;

	/**
	 * Bitmask for the optional way feature "house number".
	 */
	static const int WAY_FEATURE_HOUSE_NUMBER = 0x40;

	/**
	 * Bitmask for the optional way feature "label position".
	 */
	static const int WAY_FEATURE_LABEL_POSITION = 0x10;

	/**
	 * Bitmask for the optional way feature "name".
	 */
	static const int WAY_FEATURE_NAME = 0x80;

	/**
	 * Bitmask for the optional way feature "reference".
	 */
	static const int WAY_FEATURE_REF = 0x20;

	/**
	 * Bitmask for the way layer.
	 */
	static const int WAY_LAYER_BITMASK = 0xf0;

	/**
	 * Bit shift for calculating the way layer.
	 */
	static const int WAY_LAYER_SHIFT = 4;

	/**
	 * Bitmask for the number of way tags.
	 */
	static const int WAY_NUMBER_OF_TAGS_BITMASK = 0x0f;

	QIODevice *const m_inputFile;
	IndexCache m_databaseIndexCache;
	MapFileHeader m_mapFileHeader;
	ReadBuffer m_readBuffer;
	QString m_signatureBlock;
	QString m_signaturePoi;
	QString m_signatureWay;
};

#endif

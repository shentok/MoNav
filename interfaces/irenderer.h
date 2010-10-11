#ifndef IRENDERER_H
#define IRENDERER_H

#include "utils/coordinates.h"
#include "interfaces/irouter.h"
#include <QtPlugin>
#include <QPainter>

class IRenderer
{
public:

	struct PaintRequest {
		ProjectedCoordinate center; // center of the image
		int zoom; // required zoom level; in most rendering plugins the zoom level z will zoom in by faktor 2^z
		int virtualZoom; // scale the whole image -> neccessary for high DPI devices
		double rotation; // rotation in degrees [0,360]
		UnsignedCoordinate position; // position of the source indicator
		double heading; // heading of the source indicator
		UnsignedCoordinate target; // position of the target indicator
		QVector< UnsignedCoordinate > POIs; // a list of points of interest to highlight
		QVector< int > edgeSegments; // a list of edge segments to draw; each segment only stores the length of the segment
		QVector< UnsignedCoordinate > edges; // the sorted list of the edge segments' paths
		QVector< IRouter::Node > route; // the current route

		PaintRequest() {
			zoom = 0;
			virtualZoom = 1;
			rotation = 0;
			heading = 0;
		}
	};

	virtual QString GetName() = 0;
	virtual void SetInputDirectory( const QString& dir ) = 0;
	virtual void ShowSettings() = 0;
	virtual bool IsCompatible( int fileFormatVersion ) = 0;
	virtual bool LoadData() = 0;
	// get the maximal zoom level; possible zoom levels are: [0,GetMaxZoom()]
	virtual int GetMaxZoom() = 0;
	// modify the request to respond to a mouse movement
	virtual ProjectedCoordinate Move( int shiftX, int shiftY, const PaintRequest& request ) = 0;
	// transform a position in the image into a coordinate
	virtual ProjectedCoordinate PointToCoordinate( int shiftX, int shiftY, const PaintRequest& request ) = 0;
	// paint onto a QPainter
	virtual bool Paint( QPainter* painter, const PaintRequest& request ) = 0;
	// connect an object to the update slot; a update is emitted whenever IRenderer would render an image differently ( e.g., after fetching a tile from the internet )
	virtual void SetUpdateSlot( QObject* obj, const char* slot ) = 0;
	virtual ~IRenderer() {}
};

Q_DECLARE_INTERFACE( IRenderer, "monav.IRenderer/1.2" )

#endif // IRENDERER_H

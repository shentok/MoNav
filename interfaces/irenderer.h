#ifndef IRENDERER_H
#define IRENDERER_H

#include "utils/coordinates.h"
#include <QtPlugin>
#include <QPainter>

class IRenderer
{
public:

	virtual QString GetName() = 0;
	virtual void SetInputDirectory( const QString& dir ) = 0;
	virtual void ShowSettings() = 0;
	virtual bool LoadData() = 0;
	virtual int GetMaxZoom() = 0;
	virtual ProjectedCoordinate Move( ProjectedCoordinate center, int shiftX, int shiftY, int zoom ) = 0;
	virtual ProjectedCoordinate PointToCoordinate( ProjectedCoordinate center, int shiftX, int shiftY, int zoom ) = 0;
	virtual ProjectedCoordinate ZoomInOn( ProjectedCoordinate center, ProjectedCoordinate zoomPoint, int zoom ) = 0;
	virtual ProjectedCoordinate ZoomOutOn( ProjectedCoordinate center, ProjectedCoordinate zoomPoint, int zoom ) = 0;
	virtual bool SetPoints( std::vector< UnsignedCoordinate >* points ) = 0;
	virtual bool SetEdges( std::vector< std::vector< UnsignedCoordinate > >* edges ) = 0;
	virtual bool SetPosition( UnsignedCoordinate coordinate, double heading )  = 0;
	virtual bool Paint( QPainter* painter, ProjectedCoordinate center, int zoomLevel, double rotation = 0, double virtualZoom = 0 ) = 0;
	virtual ~IRenderer() {};
};

Q_DECLARE_INTERFACE( IRenderer, "monav.IRenderer/1.1" )

#endif // IRENDERER_H

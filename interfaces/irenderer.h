#ifndef IRENDERER_H
#define IRENDERER_H

#include "utils/coordinates.h"
#include <QtPlugin>
#include <QPainter>

class IRenderer
{
public:

	struct PaintRequest {
		ProjectedCoordinate center;
		int zoom;
		int virtualZoom;
		double rotation;
		UnsignedCoordinate position;
		double heading;
		UnsignedCoordinate target;
		QVector< UnsignedCoordinate > POIs;
		QVector< int > edgeSegments;
		QVector< UnsignedCoordinate > edges;
		QVector< UnsignedCoordinate > route;

		PaintRequest() {
			zoom = 0;
			virtualZoom = 0;
			rotation = 0;
			heading = 0;
		}
	};

	virtual QString GetName() = 0;
	virtual void SetInputDirectory( const QString& dir ) = 0;
	virtual void ShowSettings() = 0;
	virtual bool LoadData() = 0;
	virtual int GetMaxZoom() = 0;
	virtual ProjectedCoordinate Move( ProjectedCoordinate center, int shiftX, int shiftY, int zoom ) = 0;
	virtual ProjectedCoordinate PointToCoordinate( ProjectedCoordinate center, int shiftX, int shiftY, int zoom ) = 0;
	virtual ProjectedCoordinate ZoomInOn( ProjectedCoordinate center, ProjectedCoordinate zoomPoint, int zoom ) = 0;
	virtual ProjectedCoordinate ZoomOutOn( ProjectedCoordinate center, ProjectedCoordinate zoomPoint, int zoom ) = 0;
	virtual bool Paint( QPainter* painter, const PaintRequest& request ) = 0;
	virtual ~IRenderer() {}
};

Q_DECLARE_INTERFACE( IRenderer, "monav.IRenderer/1.1" )

#endif // IRENDERER_H

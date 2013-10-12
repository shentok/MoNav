#ifndef SHAPECONTAINER_H
#define SHAPECONTAINER_H

class QPainter;

class ShapeContainer
{
public:
	virtual ~ShapeContainer() {}

	virtual void render(QPainter *painter) const = 0;
};

#endif // SHAPECONTAINER_H

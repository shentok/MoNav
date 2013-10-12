#include "ShapePaintContainer.h"

#include "ShapeContainer.h"

ShapePaintContainer::ShapePaintContainer(const ShapeContainer *shapeContainer) :
	m_shapeContainer(shapeContainer)
{
}

ShapePaintContainer::~ShapePaintContainer()
{
	delete m_shapeContainer;
}

const ShapeContainer *ShapePaintContainer::shapeContainer() const
{
	return m_shapeContainer;
}

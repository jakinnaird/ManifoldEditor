/*
* ManifoldEngine
*
* Copyright (c) 2023 James Kinnaird
*/

#include "HeightmapData.hpp"

HeightmapData::HeightmapData()
	: m_Size(0), m_MinHeight(0.0f), m_MaxHeight(0.0f), m_IsModified(false)
{
	m_DirtyRegion.reset(0, 0, 0);
}

HeightmapData::~HeightmapData()
{
	clear();
}

bool HeightmapData::create(irr::u32 size, irr::f32 defaultHeight)
{
	if (size == 0)
		return false;

	m_Size = size;
	m_HeightData.reallocate(size * size);
	m_HeightData.set_used(size * size);

	// Initialize with default height
	for (irr::u32 i = 0; i < m_HeightData.size(); ++i)
		m_HeightData[i] = defaultHeight;

	m_MinHeight = m_MaxHeight = defaultHeight;
	m_IsModified = true;
	m_DirtyRegion.reset(irr::core::aabbox3di(0, 0, 0, (irr::s32)size - 1, 0, (irr::s32)size - 1));

	return true;
}

bool HeightmapData::loadFromImage(irr::video::IImage* image)
{
	if (!image)
		return false;

	const irr::core::dimension2d<irr::u32> size = image->getDimension();
	
	// Heightmap must be square
	if (size.Width != size.Height)
		return false;

	if (!create(size.Width, 0.0f))
		return false;

	// Convert image data to height values
	for (irr::u32 z = 0; z < m_Size; ++z)
	{
		for (irr::u32 x = 0; x < m_Size; ++x)
		{
			irr::video::SColor pixel = image->getPixel(x, z);
			// Use red channel for height (0-255 -> 0.0-1.0 -> scaled)
			irr::f32 height = pixel.getRed() / 255.0f * 255.0f; // Scale as needed
			setHeight(x, z, height);
		}
	}

	updateMinMaxHeight();
	return true;
}

bool HeightmapData::loadFromFile(const irr::io::path& filename, irr::video::IVideoDriver* driver)
{
	if (!driver)
		return false;

	irr::video::IImage* image = driver->createImageFromFile(filename);
	if (!image)
		return false;

	bool result = loadFromImage(image);
	image->drop();
	return result;
}

bool HeightmapData::saveToFile(const irr::io::path& filename, irr::video::IVideoDriver* driver) const
{
	if (!driver || !isValid())
		return false;

	// Create image from heightmap data
	irr::video::IImage* image = driver->createImage(irr::video::ECF_R8G8B8, irr::core::dimension2d<irr::u32>(m_Size, m_Size));
	if (!image)
		return false;

	// Convert height values to image data
	irr::f32 heightRange = m_MaxHeight - m_MinHeight;
	if (heightRange < 0.001f)
		heightRange = 1.0f; // Prevent division by zero

	for (irr::u32 z = 0; z < m_Size; ++z)
	{
		for (irr::u32 x = 0; x < m_Size; ++x)
		{
			irr::f32 normalizedHeight = (getHeight(x, z) - m_MinHeight) / heightRange;
			irr::u8 heightValue = (irr::u8)(normalizedHeight * 255.0f);
			irr::video::SColor color(255, heightValue, heightValue, heightValue);
			image->setPixel(x, z, color);
		}
	}

	bool result = driver->writeImageToFile(image, filename);
	image->drop();
	return result;
}

void HeightmapData::clear()
{
	m_HeightData.clear();
	m_Size = 0;
	m_MinHeight = m_MaxHeight = 0.0f;
	m_IsModified = false;
	m_DirtyRegion.reset(0, 0, 0);
}

irr::f32 HeightmapData::getHeight(irr::u32 x, irr::u32 z) const
{
	if (!isValidCoordinate(x, z))
		return 0.0f;

	return m_HeightData[getIndex(x, z)];
}

irr::f32 HeightmapData::getHeightSafe(irr::s32 x, irr::s32 z) const
{
	if (x < 0 || z < 0 || (irr::u32)x >= m_Size || (irr::u32)z >= m_Size)
		return 0.0f;

	return m_HeightData[getIndex((irr::u32)x, (irr::u32)z)];
}

irr::f32 HeightmapData::getInterpolatedHeight(irr::f32 x, irr::f32 z) const
{
	if (!isValid())
		return 0.0f;

	// Clamp to valid range
	if (x < 0.0f) x = 0.0f;
	if (z < 0.0f) z = 0.0f;
	if (x >= (irr::f32)(m_Size - 1)) x = (irr::f32)(m_Size - 1) - 0.001f;
	if (z >= (irr::f32)(m_Size - 1)) z = (irr::f32)(m_Size - 1) - 0.001f;

	// Get integer coordinates
	irr::u32 x0 = (irr::u32)x;
	irr::u32 z0 = (irr::u32)z;
	irr::u32 x1 = x0 + 1;
	irr::u32 z1 = z0 + 1;

	// Get fractional parts
	irr::f32 fx = x - (irr::f32)x0;
	irr::f32 fz = z - (irr::f32)z0;

	// Get height values at corners
	irr::f32 h00 = getHeight(x0, z0);
	irr::f32 h10 = getHeight(x1, z0);
	irr::f32 h01 = getHeight(x0, z1);
	irr::f32 h11 = getHeight(x1, z1);

	// Bilinear interpolation
	irr::f32 h0 = h00 * (1.0f - fx) + h10 * fx;
	irr::f32 h1 = h01 * (1.0f - fx) + h11 * fx;
	return h0 * (1.0f - fz) + h1 * fz;
}

void HeightmapData::setHeight(irr::u32 x, irr::u32 z, irr::f32 height)
{
	if (!isValidCoordinate(x, z))
		return;

	m_HeightData[getIndex(x, z)] = height;
	markRegionDirty(x, z);

	// Update min/max
	if (height < m_MinHeight)
		m_MinHeight = height;
	if (height > m_MaxHeight)
		m_MaxHeight = height;
}

bool HeightmapData::setHeightSafe(irr::s32 x, irr::s32 z, irr::f32 height)
{
	if (x < 0 || z < 0 || (irr::u32)x >= m_Size || (irr::u32)z >= m_Size)
		return false;

	setHeight((irr::u32)x, (irr::u32)z, height);
	return true;
}

bool HeightmapData::updateRegion(irr::u32 startX, irr::u32 startZ, irr::u32 width, irr::u32 height, const irr::f32* heightData)
{
	if (!heightData || !isValid())
		return false;

	// Clamp region to valid bounds
	if (startX >= m_Size || startZ >= m_Size)
		return false;

	if (startX + width > m_Size)
		width = m_Size - startX;
	if (startZ + height > m_Size)
		height = m_Size - startZ;

	// Update height data
	for (irr::u32 z = 0; z < height; ++z)
	{
		for (irr::u32 x = 0; x < width; ++x)
		{
			irr::f32 newHeight = heightData[z * width + x];
			setHeight(startX + x, startZ + z, newHeight);
		}
	}

	markRegionDirty(startX, startZ, width, height);
	updateMinMaxHeight();
	return true;
}

void HeightmapData::getRegion(irr::u32 startX, irr::u32 startZ, irr::u32 width, irr::u32 height, irr::f32* heightData) const
{
	if (!heightData || !isValid())
		return;

	// Clamp region to valid bounds
	if (startX >= m_Size || startZ >= m_Size)
		return;

	if (startX + width > m_Size)
		width = m_Size - startX;
	if (startZ + height > m_Size)
		height = m_Size - startZ;

	// Copy height data
	for (irr::u32 z = 0; z < height; ++z)
	{
		for (irr::u32 x = 0; x < width; ++x)
		{
			heightData[z * width + x] = getHeight(startX + x, startZ + z);
		}
	}
}

void HeightmapData::markRegionDirty(irr::u32 x, irr::u32 z, irr::u32 width, irr::u32 height)
{
	m_IsModified = true;

	if (m_DirtyRegion.isEmpty())
	{
		m_DirtyRegion.reset(irr::core::aabbox3di((irr::s32)x, 0, (irr::s32)z, 
			(irr::s32)(x + width - 1), 0, (irr::s32)(z + height - 1)));
	}
	else
	{
		// Expand dirty region
		if ((irr::s32)x < m_DirtyRegion.MinEdge.X)
			m_DirtyRegion.MinEdge.X = (irr::s32)x;
		if ((irr::s32)z < m_DirtyRegion.MinEdge.Z)
			m_DirtyRegion.MinEdge.Z = (irr::s32)z;
		if ((irr::s32)(x + width - 1) > m_DirtyRegion.MaxEdge.X)
			m_DirtyRegion.MaxEdge.X = (irr::s32)(x + width - 1);
		if ((irr::s32)(z + height - 1) > m_DirtyRegion.MaxEdge.Z)
			m_DirtyRegion.MaxEdge.Z = (irr::s32)(z + height - 1);
	}
}

void HeightmapData::smooth(irr::u32 iterations)
{
	if (!isValid() || iterations == 0)
		return;

	smoothRegion(0, 0, m_Size, m_Size, iterations);
}

void HeightmapData::smoothRegion(irr::u32 startX, irr::u32 startZ, irr::u32 width, irr::u32 height, irr::u32 iterations)
{
	if (!isValid() || iterations == 0)
		return;

	// Clamp region to valid bounds
	if (startX >= m_Size || startZ >= m_Size)
		return;

	if (startX + width > m_Size)
		width = m_Size - startX;
	if (startZ + height > m_Size)
		height = m_Size - startZ;

	// Create temporary buffer
	irr::core::array<irr::f32> tempData;
	tempData.reallocate(width * height);
	tempData.set_used(width * height);

	for (irr::u32 iter = 0; iter < iterations; ++iter)
	{
		// Copy current region data
		getRegion(startX, startZ, width, height, tempData.pointer());

		// Apply smoothing
		for (irr::u32 z = 1; z < height - 1; ++z)
		{
			for (irr::u32 x = 1; x < width - 1; ++x)
			{
				irr::f32 sum = 0.0f;
				irr::u32 count = 0;

				// Average surrounding pixels
				for (irr::s32 dz = -1; dz <= 1; ++dz)
				{
					for (irr::s32 dx = -1; dx <= 1; ++dx)
					{
						sum += tempData[(z + dz) * width + (x + dx)];
						count++;
					}
				}

				setHeight(startX + x, startZ + z, sum / (irr::f32)count);
			}
		}
	}

	markRegionDirty(startX, startZ, width, height);
	updateMinMaxHeight();
}

void HeightmapData::normalizeHeights()
{
	if (!isValid())
		return;

	updateMinMaxHeight();
	irr::f32 range = m_MaxHeight - m_MinHeight;
	if (range < 0.001f)
		return;

	for (irr::u32 i = 0; i < m_HeightData.size(); ++i)
	{
		m_HeightData[i] = (m_HeightData[i] - m_MinHeight) / range;
	}

	m_MinHeight = 0.0f;
	m_MaxHeight = 1.0f;
	m_IsModified = true;
	m_DirtyRegion.reset(irr::core::aabbox3di(0, 0, 0, (irr::s32)m_Size - 1, 0, (irr::s32)m_Size - 1));
}

void HeightmapData::scaleHeights(irr::f32 scale)
{
	if (!isValid())
		return;

	for (irr::u32 i = 0; i < m_HeightData.size(); ++i)
	{
		m_HeightData[i] *= scale;
	}

	m_MinHeight *= scale;
	m_MaxHeight *= scale;
	m_IsModified = true;
	m_DirtyRegion.reset(irr::core::aabbox3di(0, 0, 0, (irr::s32)m_Size - 1, 0, (irr::s32)m_Size - 1));
}

void HeightmapData::updateMinMaxHeight()
{
	if (!isValid())
		return;

	m_MinHeight = m_MaxHeight = m_HeightData[0];
	for (irr::u32 i = 1; i < m_HeightData.size(); ++i)
	{
		if (m_HeightData[i] < m_MinHeight)
			m_MinHeight = m_HeightData[i];
		if (m_HeightData[i] > m_MaxHeight)
			m_MaxHeight = m_HeightData[i];
	}
}

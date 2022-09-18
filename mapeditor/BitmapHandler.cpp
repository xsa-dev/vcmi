//
//  BitmapHandler.cpp
//  vcmieditor
//
//  Created by nordsoft on 29.08.2022.
//
#include "StdInc.h"
#include "BitmapHandler.h"

#include "../lib/filesystem/Filesystem.h"

#include <QBitmap>
#include <QImage>
#include <QPixmap>

namespace BitmapHandler
{
	QImage loadH3PCX(ui8 * data, size_t size);
	
	QImage loadBitmapFromDir(std::string path, std::string fname, bool setKey=true);

	bool isPCX(const ui8 *header)//check whether file can be PCX according to header
	{
		ui32 fSize  = read_le_u32(header + 0);
		ui32 width  = read_le_u32(header + 4);
		ui32 height = read_le_u32(header + 8);
		return fSize == width*height || fSize == width*height*3;
	}

	enum Epcxformat
	{
		PCX8B,
		PCX24B
	};

	QImage loadH3PCX(ui8 * pcx, size_t size)
	{
		//SDL_Surface * ret;
		
		Epcxformat format;
		int it=0;
		
		ui32 fSize = read_le_u32(pcx + it); it+=4;
		ui32 width = read_le_u32(pcx + it); it+=4;
		ui32 height = read_le_u32(pcx + it); it+=4;
		
		if (fSize==width*height*3)
			format=PCX24B;
		else if (fSize==width*height)
			format=PCX8B;
		else
			return QImage();
		
		QSize qsize(width, height);
		
		if (format==PCX8B)
		{
			it = 0xC;
			//auto bitmap = QBitmap::fromData(qsize, pcx + it);
			QImage image(pcx + it, width, height, QImage::Format_Indexed8);
			
			//palette - last 256*3 bytes
			QVector<QRgb> colorTable;
			it = (int)size-256*3;
			for (int i=0;i<256;i++)
			{
				char bytes[3];
				bytes[0] = pcx[it++];
				bytes[1] = pcx[it++];
				bytes[2] = pcx[it++];
				colorTable.append(qRgb(bytes[0], bytes[1], bytes[2]));
			}
			image.setColorTable(colorTable);
			return image;
		}
		else
		{
			QImage image(pcx + it, width, height, QImage::Format_RGB32);
			return image;
		}
	}

	QImage loadBitmapFromDir(std::string path, std::string fname, bool setKey)
	{
		if(!fname.size())
		{
			logGlobal->warn("Call to loadBitmap with void fname!");
			return QImage();
		}
		if (!CResourceHandler::get()->existsResource(ResourceID(path + fname, EResType::IMAGE)))
		{
			return QImage();
		}
		
		auto fullpath = CResourceHandler::get()->getResourceName(ResourceID(path + fname, EResType::IMAGE));
		auto readFile = CResourceHandler::get()->load(ResourceID(path + fname, EResType::IMAGE))->readAll();
		
		if (isPCX(readFile.first.get()))
		{//H3-style PCX
			auto image = BitmapHandler::loadH3PCX(readFile.first.get(), readFile.second);
			if(!image.isNull())
			{
				if(image.bitPlaneCount() == 1 && setKey)
				{
					QVector<QRgb> colorTable = image.colorTable();
					colorTable[0] = qRgba(255, 255, 255, 0);
					image.setColorTable(colorTable);
				}
			}
			else
			{
				logGlobal->error("Failed to open %s as H3 PCX!", fname);
			}
			return image;
		}
		else
		{ //loading via SDL_Image
			QImage image(QString::fromStdString(fullpath->make_preferred().string()));
			if(!image.isNull())
			{
				if(image.bitPlaneCount() == 1)
				{
					//set correct value for alpha\unused channel
					QVector<QRgb> colorTable = image.colorTable();
					for(auto & c : colorTable)
						c = qRgb(qRed(c), qGreen(c), qBlue(c));
					image.setColorTable(colorTable);
				}
			}
			else
			{
				logGlobal->error("Failed to open %s via QImage", fname);
				return image;
			}
		}
		return QImage();
		// When modifying anything here please check two use cases:
		// 1) Vampire mansion in Necropolis (not 1st color is transparent)
		// 2) Battle background when fighting on grass/dirt, topmost sky part (NO transparent color)
		// 3) New objects that may use 24-bit images for icons (e.g. witchking arts)
		/*if (ret->format->palette)
		{
			CSDL_Ext::setDefaultColorKeyPresize(ret);
		}
		else if (ret->format->Amask)
		{
			SDL_SetSurfaceBlendMode(ret, SDL_BLENDMODE_BLEND);
		}
		else // always set
		{
			CSDL_Ext::setDefaultColorKey(ret);
		}
		return ret;*/
	}

	QImage loadBitmap(std::string fname, bool setKey)
	{
		QImage image = loadBitmapFromDir("DATA/", fname, setKey);
		if(image.isNull())
		{
			image = loadBitmapFromDir("SPRITES/", fname, setKey);
			if(image.isNull())
			{
				logGlobal->error("Error: Failed to find file %s", fname);
			}
		}
		return image;
	}
}
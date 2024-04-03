#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL.h>
#include <SDL_image.h>
#include <string.h>
#include <zip.h>


//gcc src/main.c -o bin/Skybox4mc -I include -L lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -llibzip


enum freeMode
{
   RAMBUFFER,
   SDLINIT,
   SDLSURFACE,
};

struct dataToFree
{
   void *ptr;
   enum freeMode mode;
   struct dataToFree *next;
};

struct dataToFree *stdDataToFree = NULL;

void freeDataToFree(void)
{
   printf("Deallocating memory:\n");
   while (stdDataToFree != NULL)
   {
      switch (stdDataToFree->mode)
      {
      case RAMBUFFER:
         printf("    RAM buffer at %p freed.\n", stdDataToFree->ptr);
         free(stdDataToFree->ptr);
         break;

      case SDLINIT:
         printf("    SDL module closed.\n");
         SDL_Quit();
         break;

      case SDLSURFACE:
         printf("    SDL surface at %p freed.\n", stdDataToFree->ptr);
         SDL_FreeSurface(stdDataToFree->ptr);
         break;

      default:
         break;
      }

      stdDataToFree = stdDataToFree->next;
   }

   printf("All memory used by the program freed.\n\n");
   return;
}

void exitWithError(const char *msg)
{
   fprintf(stderr, msg);
   freeDataToFree();
   printf("Program cancelled.\n");
   exit(EXIT_FAILURE);
}

void addDataToFree(void *ptr, enum freeMode mode)
{
   struct dataToFree *newElement = malloc(sizeof(struct dataToFree));
   if(newElement == NULL)
      exitWithError("ERROR: potatoe not enough memory\n\n");

   newElement->ptr = ptr;
   newElement->mode = mode;
   newElement->next = stdDataToFree;

   stdDataToFree = newElement;
   return;
}

void freeData(void*ptr)
{
   struct dataToFree *currentData = stdDataToFree;
   while (currentData != NULL)
   {
      if(currentData->ptr == ptr)
      {
         switch (currentData->mode)
         {
         case RAMBUFFER:
            //printf("RAM buffer at %p freed.\n\n", currentData->ptr);
            free(currentData->ptr);
            currentData->ptr = NULL;
            break;

         case SDLINIT:
            //printf("SDL module closed.\n\n");
            SDL_Quit();
            break;

         case SDLSURFACE:
            //printf("SDL surface at %p freed.\n\n", currentData->ptr);
            SDL_FreeSurface(currentData->ptr);
            currentData->ptr = NULL;
            break;

         default:
            break;
         }
      }

      currentData = currentData->next;
   }
}


enum POVMode
{
   Bottom,
   Top,
   Back,
   Left,
   Front,
   Right
};

void getTilePad(int tileDimensions, enum POVMode mode, int *padX, int *padY)
{
   switch (mode)
   {
   case Bottom:
      *padX = 0;
      *padY = 0;
      break;
   
   case Top:
      *padX = tileDimensions;
      *padY = 0;
      break;
   
   case Back:
      *padX = tileDimensions+tileDimensions;
      *padY = 0;
      break;
   
   case Left:
      *padX = 0;
      *padY = tileDimensions;
      break;
   
   case Front:
      *padX = tileDimensions;
      *padY = tileDimensions;
      break;
   
   case Right:
      *padX = tileDimensions+tileDimensions;
      *padY = tileDimensions;
      break;

   default:
      exitWithError("ERROR: Wrong format specified in POVMode in getTilePad()\n\n");
      break;
   }
}

void getPointCoordinates(int i, int j, int tileDimensions, enum POVMode mode, double *x, double *y, double *z)
{
   switch (mode)
   {
   case Bottom:
      *x = (j+j+1)/(double)tileDimensions-1;
      *y = 1-(i+i+1)/(double)tileDimensions;
      *z = -1;
      break;
   
   case Top:
      *x = 1-(j+j+1)/(double)tileDimensions;
      *y = 1-(i+i+1)/(double)tileDimensions;
      *z = 1;
      break;
   
   case Back:
      *x = 1;
      *y = (i+i+1)/(double)tileDimensions-1;
      *z = 1-(j+j+1)/(double)tileDimensions;
      break;
   
   case Left:
      *x = 1-(i+i+1)/(double)tileDimensions;
      *y = 1;
      *z = 1-(j+j+1)/(double)tileDimensions;
      break;
   
   case Front:
      *x = -1;
      *y = 1-(i+i+1)/(double)tileDimensions;
      *z = 1-(j+j+1)/(double)tileDimensions;
      break;
   
   case Right:
      *x = (i+i+1)/(double)tileDimensions-1;
      *y = -1;
      *z = 1-(j+j+1)/(double)tileDimensions;
      break;

   default:
      exitWithError("ERROR: Wrong format specified in POVMode in getPointCoordinates()\n\n");
      break;
   }
}

double getLatitude(double x, double y, double z)
{
   return atan2(sqrt(x*x+y*y), z);
}

double getLongitude(double x, double y)
{
   return atan2(y, x);
}

double positiveModulo(double a, double b)
{
   a = fmod(a, b);
   return (a < 0) ? a+b : a;
}

void drawPOV(SDL_Surface *inputSurface, SDL_Surface *outputSurface, int outputTileDimensions, enum POVMode mode)
{
   Uint32 *outputImagePixels = outputSurface->pixels;
   Uint8 r0, g0, b0, r1, g1, b1, r2, g2, b2;
   double x, y, z, latitude, longitude, xfrac, yfrac;
   int padX, padY, xpx, ypx;

   getTilePad(outputTileDimensions, mode, &padX, &padY);

   for(int i = 0; i<outputTileDimensions; i++)
   {
      for(int j = 0; j<outputTileDimensions; j++)
      {
         getPointCoordinates(i, j, outputTileDimensions, mode, &x, &y, &z);

         latitude = getLatitude(x, y, z);
         longitude = getLongitude(x, y);

         xfrac = positiveModulo((double)inputSurface->w*longitude/2/M_PI, inputSurface->w);
         yfrac = (double)inputSurface->h*latitude/M_PI;
         xpx = floor(xfrac);
         ypx = floor(yfrac);
         xfrac -= xpx;
         yfrac -= ypx;

         SDL_GetRGB(*(Uint32*)((Uint8*)inputSurface->pixels + (ypx*inputSurface->w+xpx) * inputSurface->format->BytesPerPixel), inputSurface->format, &r0, &g0, &b0);
         SDL_GetRGB(*(Uint32*)((Uint8*)inputSurface->pixels + (ypx*inputSurface->w+(xpx+1)) * inputSurface->format->BytesPerPixel), inputSurface->format, &r1, &g1, &b1);
         r0 = r0 + xfrac * (r1 - r0);
         g0 = g0 + xfrac * (g1 - g0);
         b0 = b0 + xfrac * (b1 - b0);
         SDL_GetRGB(*(Uint32*)((Uint8*)inputSurface->pixels + ((ypx+1)*inputSurface->w+xpx) * inputSurface->format->BytesPerPixel), inputSurface->format, &r1, &g1, &b1);
         SDL_GetRGB(*(Uint32*)((Uint8*)inputSurface->pixels + ((ypx+1)*inputSurface->w+(xpx+1)) * inputSurface->format->BytesPerPixel), inputSurface->format, &r2, &g2, &b2);
         r1 = r1 + xfrac * (r2 - r1);
         g1 = g1 + xfrac * (g2 - g1);
         b1 = b1 + xfrac * (b2 - b1);
         r0 = r0 + yfrac * (r1 - r0);
         g0 = g0 + yfrac * (g1 - g0);
         b0 = b0 + yfrac * (b1 - b0);

         outputImagePixels[(j+padY) * outputSurface->w + (i+padX)] = SDL_MapRGB(outputSurface->format, r0, g0, b0);
      }
   }
}


int main(int argc, char **argv)
{
   unsigned char *inputImagePath = "./inputImage.jpg";
   unsigned char *outputArchivePath = "./outputArchive.zip";
   int outputImageTileDimensions = 2048;
   int previewImageDimensions = 256;
   int packFormat = 1;
   unsigned char *packDescription = "§7Made using §6Skybox4mc\n  §7by §bUndefinedPseudo";

   if(argc >= 2)
      inputImagePath = argv[1];
   if(argc >= 3)
      outputArchivePath = argv[2];
   if(argc >= 4 && atoi(argv[3]) > 0)
      outputImageTileDimensions = atoi(argv[3]);
   if(argc >= 5 && atoi(argv[4]) > 0)
      previewImageDimensions = atoi(argv[4]);
   if(argc >= 6 && atoi(argv[5]) > 0)
      packFormat = atoi(argv[5]);
   if(argc >= 7)
      packDescription = argv[6];
   if(argc > 7)
      exitWithError("ERROR: Wrong argument number specified when starting program\n\n");

   printf("Creating pack with:\n    inputImagePath=\"%s\",\n    outputArchivePath=\"%s\",\n    outputImageTileDimensions=%d,\n    previewImageDimensions=%d,\n    packFormat=%d,\n packDescription=\"%s\"\n...\n\n", inputImagePath, outputArchivePath, outputImageTileDimensions, previewImageDimensions, packFormat, packDescription);


   if(SDL_Init(SDL_INIT_VIDEO) != 0)
      exitWithError("ERROR: SDL initialisation\n\n");
   addDataToFree(NULL, SDLINIT);

   SDL_Surface *inputImageSurface = IMG_Load(inputImagePath);
   if(inputImageSurface == NULL)
      exitWithError("ERROR: Input image surface loading\n\n");
   addDataToFree(inputImageSurface, SDLSURFACE);


   printf("Creating sky:\n");

   SDL_Surface *outputImageSurface = SDL_CreateRGBSurfaceWithFormat(0, outputImageTileDimensions*3, outputImageTileDimensions*2, 24, SDL_PIXELFORMAT_RGB888);
   if(outputImageSurface == NULL)
      exitWithError("ERROR: Output image surface creation\n\n");
   addDataToFree(outputImageSurface, SDLSURFACE);

   if(SDL_LockSurface(inputImageSurface) != 0)
      exitWithError("ERROR: Input image surface locking\n\n");
   if(SDL_LockSurface(outputImageSurface) != 0)
      exitWithError("ERROR: Output image surface locking\n\n");

   printf("    Generating bottom tile...");
   drawPOV(inputImageSurface, outputImageSurface, outputImageTileDimensions, Bottom);
   printf(" Done.\n    Generating top tile...");
   drawPOV(inputImageSurface, outputImageSurface, outputImageTileDimensions, Top);
   printf(" Done.\n    Generating back tile...");
   drawPOV(inputImageSurface, outputImageSurface, outputImageTileDimensions, Back);
   printf(" Done.\n    Generating left tile...");
   drawPOV(inputImageSurface, outputImageSurface, outputImageTileDimensions, Left);
   printf(" Done.\n    Generating front tile...");
   drawPOV(inputImageSurface, outputImageSurface, outputImageTileDimensions, Front);
   printf(" Done.\n    Generating right tile...");
   drawPOV(inputImageSurface, outputImageSurface, outputImageTileDimensions, Right);
   printf(" Done.\n");

   SDL_UnlockSurface(inputImageSurface);
   SDL_UnlockSurface(outputImageSurface);

   printf("Sky created.\n\n");


   if(IMG_SavePNG(outputImageSurface, "outputImage.png") != 0)
      exitWithError("ERROR: Output image saving to ROM\n\n");


   freeData(outputImageSurface);


   printf("Generating pack preview...");

   SDL_Surface *previewImageSurface = SDL_CreateRGBSurfaceWithFormat(0, previewImageDimensions, previewImageDimensions, 24, SDL_PIXELFORMAT_RGB888);
   if(previewImageSurface == NULL)
      exitWithError("ERROR: Preview image surface creation\n\n");
   addDataToFree(previewImageSurface, SDLSURFACE);

   int lowerDimension = (inputImageSurface->w<inputImageSurface->h)?inputImageSurface->w:inputImageSurface->h;
   SDL_Rect rect;
   rect.x = (inputImageSurface->w-lowerDimension)/2;
   rect.y = (inputImageSurface->h-lowerDimension)/2;
   rect.w = lowerDimension;
   rect.h = lowerDimension;

   if(SDL_BlitScaled(inputImageSurface, &rect, previewImageSurface, NULL) != 0)
      exitWithError("ERROR: Preview image generation\n");

   printf(" Done.\n\n");


   if(IMG_SavePNG(previewImageSurface, "previewImage.png") != 0)
      exitWithError("ERROR: Preview image saving to ROM\n\n");


   freeData(previewImageSurface);


   freeData(inputImageSurface);


   printf("Creating pack mcmeta data...");

   unsigned char *propertiesStr = malloc((140+1)*sizeof(unsigned char));
   if(propertiesStr == NULL)
      exitWithError("ERROR: potatoe not enough memory\n\n");
   addDataToFree(propertiesStr, RAMBUFFER);

   itoa(packFormat, propertiesStr, 10);

   unsigned char *mcmetaStr = malloc((33+strlen(propertiesStr)+22+strlen(packDescription)+8+1)*sizeof(unsigned char));
   if(mcmetaStr == NULL)
      exitWithError("ERROR: potatoe not enough memory\n\n");
   addDataToFree(mcmetaStr, RAMBUFFER);

   strcpy(mcmetaStr, "{\n  \"pack\": {\n    \"pack_format\": "); //33
   strcat(mcmetaStr, propertiesStr);
   strcat(mcmetaStr, ",\n    \"description\": \""); //22
   strcat(mcmetaStr, packDescription);
   strcat(mcmetaStr, "\"\n  }\n}\n"); //8

   printf(" Done.\n\nCreating sky properties...");

   strcpy(propertiesStr, "startFadeIn=0:00\nendFadeIn=0:00\nendFadeOut=23:59\nsource=mcpatcher/sky/world0/sky1.png\nblend=replace\nrotate=false\nweather=clear rain thunder\n"); //140

   printf(" Done.\n\n");


   printf("Writing pack archive at \"%s\"...", outputArchivePath);

   struct zip *outputArchive = NULL;
   int error = 0;

   outputArchive = zip_open(outputArchivePath, ZIP_TRUNCATE|ZIP_CREATE, &error);
   if(error != 0)
      exitWithError("ERROR: Output archive opening or creation\n\n");

   struct zip_source *mcmetaSource = zip_source_buffer(outputArchive, mcmetaStr, strlen(mcmetaStr)*sizeof(unsigned char), 0);
   if(mcmetaSource == NULL)
      exitWithError("ERROR: Zip source buffer creation\n\n");
   if(zip_file_add(outputArchive, "pack.mcmeta", mcmetaSource, ZIP_FL_OVERWRITE) < 0)
      exitWithError("ERROR: File addition to the archive\n\n");

   struct zip_source *previewSource = zip_source_file(outputArchive, "./previewImage.png", 0, 0);
   if(previewSource == NULL)
      exitWithError("ERROR: Zip source file creation\n\n");
   if(zip_file_add(outputArchive, "pack.png", previewSource, ZIP_FL_OVERWRITE) < 0)
      exitWithError("ERROR: File addition to the archive\n\n");

   if(zip_dir_add(outputArchive, "assets/minecraft/mcpatcher/sky/world0", 0) == -1)
      exitWithError("ERROR: Directory addition to the archive\n\n");
   if(zip_dir_add(outputArchive, "assets/minecraft/optifine/sky/world0", 0) == -1)
      exitWithError("ERROR: Directory addition to the archive\n\n");

   struct zip_source *skySource = zip_source_file(outputArchive, "./outputImage.png", 0, 0);
   if(skySource == NULL)
      exitWithError("ERROR: Zip source file creation\n\n");
   if(zip_file_add(outputArchive, "assets/minecraft/mcpatcher/sky/world0/sky1.png", skySource, ZIP_FL_OVERWRITE) < 0)
      exitWithError("ERROR: File addition to the archive\n\n");

   struct zip_source *propertiesSource = zip_source_buffer(outputArchive, propertiesStr, strlen(propertiesStr)*sizeof(unsigned char), 0);
   if(propertiesSource == NULL)
      exitWithError("ERROR: Zip source buffer creation\n\n");
   zip_source_keep(propertiesSource);
   if(zip_file_add(outputArchive, "assets/minecraft/mcpatcher/sky/world0/sky1.properties", propertiesSource, ZIP_FL_OVERWRITE) < 0)
      exitWithError("ERROR: File addition to the archive\n\n");
   if(zip_file_add(outputArchive, "assets/minecraft/optifine/sky/world0/sky1.properties", propertiesSource, ZIP_FL_OVERWRITE) < 0)
      exitWithError("ERROR: File addition to the archive\n\n");

   zip_close(outputArchive);

   printf(" Done.\n\n");


   freeData(mcmetaStr);

   freeData(propertiesStr);


   freeDataToFree();

   printf("Program ended successfully.\n\n");
   return EXIT_SUCCESS;
}

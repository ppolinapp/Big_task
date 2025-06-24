#include <stdio.h>
#include <stdlib.h> 
#include <strings.h>
#include <math.h> 
#include "lodepng.h"
#define min_color 15

//функция для загрузки изображения
unsigned char* load_png(const char* filename, unsigned int* width, unsigned int* height) 
{
  unsigned char* image = NULL; 
  int error = lodepng_decode32_file(&image, width, height, filename);
  if(error != 0) {
    printf("error %u: %s\n", error, lodepng_error_text(error)); 
  }
  return (image);
}

//функция для записи изображения
void write_png(const char* filename, const unsigned char* image, unsigned width, unsigned height)
{
  unsigned char* png;
  long unsigned int pngsize;
  int error = lodepng_encode32(&png, &pngsize, image, width, height);
  if(error == 0) {
      lodepng_save_file(png, pngsize, filename);
  } else { 
    printf("error %u: %s\n", error, lodepng_error_text(error));
  }
  free(png);
}

//функция для перевода изображения в чёрно-белый формат
void to_wb(unsigned char* image, unsigned char* wb_image,  unsigned width,  unsigned height)
  {
    int i, j, p;
    for (i = 0; i < height; i++)
      for (j = 0; j < width; j++){
        p = i*width + j;
        wb_image[p] = (image[p*4] + image[p*4 + 1] + image[p*4 + 2]) / 3;
      }
      return;
  }

  //функция для повышения контраста изображения (очень серые пиксели становятся чёрными, а наименее серые - белыми)
  void contrast(unsigned char *col, int bw_size){
    int i; 
    for(i=0; i < bw_size; i++)
    {
        if(col[i] < 25)
        col[i] = 0; 
        if(col[i] > 195)
        col[i] = 255;
    } 
    return; 
} 

//фильтр Гаусса(размытие изображения, уменьшение шумов)
  void Gauss_blur(unsigned char *wb_image, unsigned char *gauss_image, int width, int height)
  { 
      int i, j; 
      for(i = 1; i < height - 1; i++)  
          for(j = 1; j < width - 1; j++)
          { 
              gauss_image[width*i+j] = 0.084*wb_image[width*i+j] + 0.084*wb_image[width*(i+1)+j] + 0.084*wb_image[width*(i-1)+j]; 
              gauss_image[width*i+j] = gauss_image[width*i+j] + 0.084*wb_image[width*i+(j+1)] + 0.084*wb_image[width*i+(j-1)]; 
              gauss_image[width*i+j] = gauss_image[width*i+j] + 0.063*wb_image[width*(i+1)+(j+1)] + 0.063*wb_image[width*(i+1)+(j-1)]; 
              gauss_image[width*i+j] = gauss_image[width*i+j] + 0.063*wb_image[width*(i-1)+(j+1)] + 0.063*wb_image[width*(i-1)+(j-1)]; 
          } 
     return;
    }

  //фильтр Собеля (выделение границ)
  void Sobel(unsigned char *filtered_image, unsigned char *sobel_image, int width, int height){
    int i, j;
    float gx, gy;
    for(i = 1; i < height - 1; i++)  
          for(j = 1; j < width - 1; j++){
            gx = -1 * filtered_image[(i - 1)*width + (j - 1)]
                 - 2 * filtered_image[i * width + (j - 1)]
                 - 1 * filtered_image[(i + 1)*width + (j - 1)]
                 + 1 * filtered_image[(i - 1)*width + (j + 1)]
                 + 2 * filtered_image[i*width + (j + 1)]
                 + 1 * filtered_image[(i + 1)*width + (j + 1)];
            gy = - 1 * filtered_image[(i + 1)*width + (j - 1)]
                 - 2 * filtered_image[(i + 1) * width + j]
                 - 1 * filtered_image[(i + 1)*width + (j + 1)]
                 + 1 * filtered_image[(i - 1)*width + (j - 1)]
                 + 2 * filtered_image[(i - 1)*width + j]
                 + 1 * filtered_image[(i - 1)*width + (j + 1)];
            sobel_image[i*width + j] = sqrt(gx*gx + gy*gy);
            if (sobel_image[i*width + j] >= 255)
              sobel_image[i*width + j] = 255;
          };
  }
  //структура для элемента системы непересекающихся множеств
  typedef struct dsu_elem {int parent;
                int val;} dsu_elem;
  
  //функция инициализации DSU
  void init(unsigned char* filtered_image, dsu_elem* DSU, int width, int height){
    int i, j, p;
    for (i = 0; i < height; i++)
      for (j = 0; j < width; j++){
        p = i*width + j;
        DSU[p].parent = p;
        DSU[p].val = filtered_image[p];
      }
      return;
  }
  
  //функция поиска представителя 
  int find_set(dsu_elem* elem, int n, dsu_elem* DSU){
    if (n != elem->parent)
      return elem->parent = find_set(&DSU[elem->parent], elem->parent, DSU);
    return n;
  }
  
  //функция объединения двух множеств
  void unite(dsu_elem* el1, dsu_elem* el2, int n1, int n2, dsu_elem* DSU){
    el1 = &DSU[find_set(el1, n1, DSU)];
    el2 = &DSU[find_set(el2, n2, DSU)];
    el1->parent = el2->parent;
    return;
  }

  //функция разбиения на компоненты связности
  void convert(unsigned char* filtered_image, dsu_elem* DSU, int width, int height){
    int i, j, di, dj, p, p1;
    init(filtered_image, DSU, width, height);
      for(i = 1; i < height - 1; i++)  
        for(j = 1; j < width - 1; j++){
          p = i*width + j;
          for(di = -1; di <= 1; di++)
            for(dj = -1; dj <= 1; dj++){
              p1 = (i + di)*width + (j + dj);
              if(filtered_image[p] > min_color && filtered_image[p1] > min_color && abs(di)
              + abs(dj) == 1 && abs(filtered_image[p] - filtered_image[p1]) < 20)
                unite(&DSU[p], &DSU[p1], p, p1, DSU);
            }
        }
  }

  //функция для раскраски изображения
void color(dsu_elem* DSU, int width, int height, unsigned char* output_image){
  int i, j, p, dip;
  //все цвета, которые исполязуются
  int colors[11][3] ={
    {255,209,220}, {239,169,74}, {127,181,181}, {255,155,170}, {252,232,131}, {204,204,255},
    {175,238,238}, {214, 255, 166}, {166, 184, 255}, {161,133,148}, {162,162,208}
  };
  for(i = 0; i < height; i++)  
        for(j = 0; j < width; j++){
          p = i*width + j;
          dip = find_set(&DSU[p], p, DSU);
          if(DSU[p].val <= min_color){
            output_image[p*4] = 0;
          output_image[p*4 + 1] = 0;
          output_image[p*4 + 2] = 0;
          output_image[p*4 + 3] = 255;
          }
          else {
          output_image[p*4] = colors[dip%11][0];
          output_image[p*4 + 1] = colors[dip%11][1];
          output_image[p*4 + 2] = colors[dip%11][2];
          output_image[p*4 + 3] = 255;
          }
        }
}

  int main() 
{ 
  const char* filename = "six.png"; 
  unsigned int width, height;
  int size;
  int bw_size;
    
  unsigned char* picture = load_png(filename, &width, &height);
  printf("%d %d", width, height);
  //write_png("first.png", picture, width, height);

  unsigned char* wb_image = malloc(sizeof(unsigned char) * width * height);
  to_wb(picture, wb_image, width, height);

  unsigned char* gauss_image = malloc(sizeof(unsigned char) * width * height);
  Gauss_blur(wb_image, gauss_image, width, height);

  unsigned char* sobel_image = malloc(sizeof(unsigned char) * width * height);
  Sobel(gauss_image, sobel_image, width, height);

  contrast(sobel_image, width*height);

  dsu_elem* DSU = malloc(sizeof(dsu_elem)*width*height);
  convert(sobel_image, DSU, width, height);

  unsigned char* output_image = malloc(sizeof(unsigned char) * 4 * width * height);
  color(DSU, width, height, output_image);

  write_png("Final_Skull_6.png", output_image, width, height);

  free(sobel_image);
  free(output_image);
  free(wb_image);
  free(gauss_image);
  free(DSU);

return 0;
}


#include "stdio.h"
#include "camera_index_ov2640.h"
#include "camera_index_ov3660.h"


void main() {
  FILE *fp2;
  FILE *fp3;
  fp2 = fopen ("ov2640_html.gz", "w");
  fp3 = fopen ("ov3660_html.gz", "w");

  for (int i = 0; i <= index_ov2640_html_gz_len; i++)
  {
    fputc(index_ov2640_html_gz[i],fp2);
  }

  for (int i = 0; i <= index_ov3660_html_gz_len; i++)
  {
    fputc(index_ov3660_html_gz[i],fp3);
  }
}


#include "../include/wmspal.h"

#ifdef HAVE_PROJ
#include <proj.h>
#endif

int georeference_image(const char* input_file, const char* output_file, const char* bbox, const char* srs) {
    printf("Creating georeferencing metadata for %s\n", input_file);
    
    char metadata_file[512];
    snprintf(metadata_file, sizeof(metadata_file), "%s.wld", output_file);
    
    double minx, miny, maxx, maxy;
    if (sscanf(bbox, "%lf,%lf,%lf,%lf", &minx, &miny, &maxx, &maxy) != 4) {
        fprintf(stderr, "Invalid bbox format. Expected: minx,miny,maxx,maxy\n");
        return 1;
    }
    
    FILE* wld_file = fopen(metadata_file, "w");
    if (!wld_file) {
        fprintf(stderr, "Failed to create world file: %s\n", metadata_file);
        return 1;
    }
    
    double pixel_size_x = (maxx - minx) / 256.0;  // Assuming 256px default
    double pixel_size_y = -(maxy - miny) / 256.0;
    
    fprintf(wld_file, "%.10f\n", pixel_size_x);  // pixel size in x direction
    fprintf(wld_file, "0.0\n");                  // rotation term
    fprintf(wld_file, "0.0\n");                  // rotation term  
    fprintf(wld_file, "%.10f\n", pixel_size_y);  // pixel size in y direction
    fprintf(wld_file, "%.10f\n", minx + pixel_size_x/2.0);  // x coordinate of center of upper left pixel
    fprintf(wld_file, "%.10f\n", maxy + pixel_size_y/2.0);  // y coordinate of center of upper left pixel
    
    fclose(wld_file);
    
    // Create projection file
    char prj_file[512];
    snprintf(prj_file, sizeof(prj_file), "%s.prj", output_file);
    FILE* proj_file = fopen(prj_file, "w");
    if (proj_file) {
#ifdef HAVE_PROJ
        // Use PROJ to get proper WKT format
        PJ_CONTEXT *ctx = proj_context_create();
        PJ *P = proj_create(ctx, srs);
        
        if (P) {
            const char *wkt = proj_as_wkt(ctx, P, PJ_WKT1_GDAL, NULL);
            if (wkt) {
                fprintf(proj_file, "%s\n", wkt);
                printf("Created projection file with PROJ WKT: %s\n", prj_file);
            } else {
                fprintf(proj_file, "%s\n", srs);
                printf("Created projection file with basic SRS: %s\n", prj_file);
            }
            proj_destroy(P);
        } else {
            fprintf(proj_file, "%s\n", srs);
        }
        proj_context_destroy(ctx);
#else
        fprintf(proj_file, "%s\n", srs);
        printf("Created projection file (basic): %s\n", prj_file);
#endif
        fclose(proj_file);
    }
    
    printf("Created world file: %s\n", metadata_file);
    
    // Copy original image to output location
    char copy_cmd[1024];
    snprintf(copy_cmd, sizeof(copy_cmd), "cp \"%s\" \"%s\"", input_file, output_file);
    int result = system(copy_cmd);
    
    if (result == 0) {
        printf("Image copied to: %s\n", output_file);
        return 0;
    } else {
        fprintf(stderr, "Failed to copy image file\n");
        return 1;
    }
}
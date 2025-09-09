#ifndef WMSPAL_H
#define WMSPAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char* url;
    char* layer;
    char* bbox;
    char* srs;
    int width;
    int height;
    char* format;
    char* output_file;
    bool vectorize;
    bool vectorize_enhanced;
    bool vectorize_geological;
    bool attribution;
    bool capabilities;
    bool raw_xml;
} wms_config_t;

typedef struct {
    unsigned char* data;
    int width;
    int height;
    int channels;
} image_t;

typedef struct {
    unsigned char r, g, b;
} color_t;

typedef struct {
    double x, y;
} coord_t;

typedef struct {
    coord_t* coords;
    int count;
    int capacity;
} polygon_t;

typedef struct {
    color_t dominant_color;
    polygon_t* polygons;
    int polygon_count;
    char* feature_info;
    char* geological_unit;
    char* age;
    char* lithology;
} geological_feature_t;

typedef struct {
    geological_feature_t* features;
    int feature_count;
    double minx, miny, maxx, maxy;  // Bounding box
    char* crs;
} vectorization_result_t;

int download_wms_tile(const wms_config_t* config);
int get_wms_capabilities(const wms_config_t* config);
int georeference_image(const char* input_file, const char* output_file, const char* bbox, const char* srs);
int vectorize_image(const char* input_file, const char* output_file);
int vectorize_geological_map(const char* input_file, const char* output_file, const wms_config_t* config);
int apply_attribution(const char* vector_file, const wms_config_t* config);

// Enhanced vectorization functions
vectorization_result_t* analyze_geological_colors(const char* image_file, const char* bbox, const char* srs);
int get_feature_info_at_point(const wms_config_t* config, double x, double y, char** result);
int write_geojson(const vectorization_result_t* result, const char* output_file);
void free_vectorization_result(vectorization_result_t* result);

// Image processing functions
image_t* load_png_simple(const char* filename);
void free_image(image_t* img);
int detect_edges_simple(image_t* img, unsigned char threshold);
color_t* extract_unique_colors(image_t* img, int* color_count);
polygon_t* trace_color_regions(image_t* img, color_t target_color, int* polygon_count);

#endif
#include "../include/wmspal.h"
#include <math.h>
#include <stdbool.h>

#ifdef HAVE_GEOS
#include <geos_c.h>
#endif

// Simple image loading for PNG (basic implementation)
image_t* load_png_simple(const char* filename) {
    // This is a placeholder - in a real implementation you'd use libpng or stb_image
    image_t* img = malloc(sizeof(image_t));
    if (!img) return NULL;
    
    FILE* file = fopen(filename, "rb");
    if (!file) {
        free(img);
        return NULL;
    }
    
    // For now, create a dummy 256x256 RGB image for testing
    img->width = 256;
    img->height = 256;
    img->channels = 3;
    img->data = malloc(img->width * img->height * img->channels);
    
    // Fill with test pattern (geological-like colors)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int idx = (y * img->width + x) * 3;
            if (x < 128 && y < 128) {
                // Reddish area (sandstone)
                img->data[idx] = 180; img->data[idx+1] = 120; img->data[idx+2] = 80;
            } else if (x >= 128 && y < 128) {
                // Bluish area (limestone)
                img->data[idx] = 100; img->data[idx+1] = 150; img->data[idx+2] = 200;
            } else {
                // Greenish area (shale)
                img->data[idx] = 120; img->data[idx+1] = 180; img->data[idx+2] = 100;
            }
        }
    }
    
    fclose(file);
    return img;
}

void free_image(image_t* img) {
    if (img) {
        if (img->data) free(img->data);
        free(img);
    }
}

// Color analysis functions
static double color_distance(color_t a, color_t b) {
    double dr = a.r - b.r;
    double dg = a.g - b.g;
    double db = a.b - b.b;
    return sqrt(dr*dr + dg*dg + db*db);
}

color_t* extract_unique_colors(image_t* img, int* color_count) {
    if (!img || !img->data) return NULL;
    
    // Simple color clustering with tolerance
    const int MAX_COLORS = 50;
    const double COLOR_TOLERANCE = 30.0;
    
    color_t* colors = malloc(MAX_COLORS * sizeof(color_t));
    int count = 0;
    
    for (int y = 0; y < img->height; y += 4) {  // Sample every 4th pixel
        for (int x = 0; x < img->width; x += 4) {
            int idx = (y * img->width + x) * img->channels;
            color_t pixel = {img->data[idx], img->data[idx+1], img->data[idx+2]};
            
            // Check if this color is already in our list
            bool found = false;
            for (int i = 0; i < count; i++) {
                if (color_distance(pixel, colors[i]) < COLOR_TOLERANCE) {
                    found = true;
                    break;
                }
            }
            
            if (!found && count < MAX_COLORS) {
                colors[count++] = pixel;
            }
        }
    }
    
    *color_count = count;
    printf("Extracted %d unique colors from geological map\n", count);
    return colors;
}

// Simple flood fill for color region tracing
static void flood_fill_region(image_t* img, int start_x, int start_y, color_t target, 
                             bool* visited, polygon_t* polygon) {
    if (start_x < 0 || start_x >= img->width || start_y < 0 || start_y >= img->height) return;
    if (visited[start_y * img->width + start_x]) return;
    
    int idx = (start_y * img->width + start_x) * img->channels;
    color_t pixel = {img->data[idx], img->data[idx+1], img->data[idx+2]};
    
    if (color_distance(pixel, target) > 20.0) return;
    
    visited[start_y * img->width + start_x] = true;
    
    // Add point to polygon (simplified boundary tracing)
    if (polygon->count >= polygon->capacity) {
        polygon->capacity *= 2;
        polygon->coords = realloc(polygon->coords, polygon->capacity * sizeof(coord_t));
    }
    
    polygon->coords[polygon->count].x = start_x;
    polygon->coords[polygon->count].y = start_y;
    polygon->count++;
    
    // Recursively fill neighbors (simplified - real implementation would use queue)
    if (polygon->count < 1000) {  // Prevent stack overflow
        flood_fill_region(img, start_x+1, start_y, target, visited, polygon);
        flood_fill_region(img, start_x-1, start_y, target, visited, polygon);
        flood_fill_region(img, start_x, start_y+1, target, visited, polygon);
        flood_fill_region(img, start_x, start_y-1, target, visited, polygon);
    }
}

polygon_t* trace_color_regions(image_t* img, color_t target_color, int* polygon_count) {
    if (!img || !img->data) return NULL;
    
    bool* visited = calloc(img->width * img->height, sizeof(bool));
    polygon_t* polygons = malloc(10 * sizeof(polygon_t));  // Max 10 regions per color
    int count = 0;
    
    for (int y = 0; y < img->height && count < 10; y += 8) {
        for (int x = 0; x < img->width && count < 10; x += 8) {
            if (!visited[y * img->width + x]) {
                int idx = (y * img->width + x) * img->channels;
                color_t pixel = {img->data[idx], img->data[idx+1], img->data[idx+2]};
                
                if (color_distance(pixel, target_color) < 20.0) {
                    polygon_t polygon = {0};
                    polygon.capacity = 100;
                    polygon.coords = malloc(polygon.capacity * sizeof(coord_t));
                    
                    flood_fill_region(img, x, y, target_color, visited, &polygon);
                    
                    if (polygon.count > 10) {  // Only keep significant regions
                        polygons[count++] = polygon;
                    } else {
                        free(polygon.coords);
                    }
                }
            }
        }
    }
    
    free(visited);
    *polygon_count = count;
    return polygons;
}

// Coordinate transformation from pixel to geographic coordinates
static coord_t pixel_to_geo(int px, int py, int width, int height, 
                           double minx, double miny, double maxx, double maxy) {
    coord_t geo;
    geo.x = minx + ((double)px / width) * (maxx - minx);
    geo.y = maxy - ((double)py / height) * (maxy - miny);  // Flip Y axis
    return geo;
}

// Enhanced geological vectorization
vectorization_result_t* analyze_geological_colors(const char* image_file, const char* bbox, const char* srs) {
    printf("Analyzing geological colors in: %s\n", image_file);
    
    // Parse bounding box
    double minx, miny, maxx, maxy;
    if (sscanf(bbox, "%lf,%lf,%lf,%lf", &minx, &miny, &maxx, &maxy) != 4) {
        fprintf(stderr, "Invalid bbox format\n");
        return NULL;
    }
    
    image_t* img = load_png_simple(image_file);
    if (!img) {
        fprintf(stderr, "Failed to load image: %s\n", image_file);
        return NULL;
    }
    
    vectorization_result_t* result = malloc(sizeof(vectorization_result_t));
    result->minx = minx; result->miny = miny;
    result->maxx = maxx; result->maxy = maxy;
    result->crs = strdup(srs);
    result->feature_count = 0;
    result->features = NULL;
    
    // Extract unique colors
    int color_count;
    color_t* colors = extract_unique_colors(img, &color_count);
    
    if (colors && color_count > 0) {
        result->features = malloc(color_count * sizeof(geological_feature_t));
        
        for (int i = 0; i < color_count; i++) {
            geological_feature_t* feature = &result->features[result->feature_count];
            feature->dominant_color = colors[i];
            feature->feature_info = NULL;
            feature->geological_unit = NULL;
            feature->age = NULL;
            feature->lithology = NULL;
            
            // Trace polygons for this color
            int polygon_count;
            polygon_t* polygons = trace_color_regions(img, colors[i], &polygon_count);
            
            if (polygons && polygon_count > 0) {
                feature->polygons = malloc(polygon_count * sizeof(polygon_t));
                feature->polygon_count = polygon_count;
                
                // Convert pixel coordinates to geographic coordinates
                for (int j = 0; j < polygon_count; j++) {
                    feature->polygons[j] = polygons[j];
                    for (int k = 0; k < polygons[j].count; k++) {
                        coord_t geo = pixel_to_geo(
                            (int)polygons[j].coords[k].x, (int)polygons[j].coords[k].y,
                            img->width, img->height, minx, miny, maxx, maxy);
                        feature->polygons[j].coords[k] = geo;
                    }
                }
                
                result->feature_count++;
                printf("Color %d: RGB(%d,%d,%d) -> %d polygons\n", 
                       i, colors[i].r, colors[i].g, colors[i].b, polygon_count);
            }
        }
    }
    
    if (colors) free(colors);
    free_image(img);
    
    printf("Geological analysis complete: %d features found\n", result->feature_count);
    return result;
}

void free_vectorization_result(vectorization_result_t* result) {
    if (!result) return;
    
    for (int i = 0; i < result->feature_count; i++) {
        geological_feature_t* feature = &result->features[i];
        if (feature->feature_info) free(feature->feature_info);
        if (feature->geological_unit) free(feature->geological_unit);
        if (feature->age) free(feature->age);
        if (feature->lithology) free(feature->lithology);
        
        for (int j = 0; j < feature->polygon_count; j++) {
            if (feature->polygons[j].coords) free(feature->polygons[j].coords);
        }
        if (feature->polygons) free(feature->polygons);
    }
    
    if (result->features) free(result->features);
    if (result->crs) free(result->crs);
    free(result);
}

// Enhanced geological vectorization workflow
int vectorize_geological_map(const char* input_file, const char* output_file, const wms_config_t* config) {
    printf("Starting comprehensive geological vectorization...\n");
    
    // Analyze colors and create geological features
    vectorization_result_t* result = analyze_geological_colors(input_file, config->bbox, config->srs);
    if (!result) {
        fprintf(stderr, "Failed to analyze geological features\n");
        return 1;
    }
    
    // Query GetFeatureInfo for each feature's centroid
    for (int i = 0; i < result->feature_count; i++) {
        geological_feature_t* feature = &result->features[i];
        
        if (feature->polygon_count > 0 && feature->polygons[0].count > 0) {
            // Calculate centroid of first polygon
            double cx = 0, cy = 0;
            for (int j = 0; j < feature->polygons[0].count; j++) {
                cx += feature->polygons[0].coords[j].x;
                cy += feature->polygons[0].coords[j].y;
            }
            cx /= feature->polygons[0].count;
            cy /= feature->polygons[0].count;
            
            // Get feature info at centroid
            char* feature_info = NULL;
            if (get_feature_info_at_point(config, cx, cy, &feature_info) == 0 && feature_info) {
                feature->feature_info = feature_info;
                
                // Parse feature information (generic approach)
                if (strstr(feature_info, "sandstone") || strstr(feature_info, "Sandstone")) {
                    feature->lithology = strdup("Sandstone");
                } else if (strstr(feature_info, "limestone") || strstr(feature_info, "Limestone")) {
                    feature->lithology = strdup("Limestone");
                } else if (strstr(feature_info, "shale") || strstr(feature_info, "Shale")) {
                    feature->lithology = strdup("Shale");
                } else if (strstr(feature_info, "water") || strstr(feature_info, "Water")) {
                    feature->lithology = strdup("Water");
                } else if (strstr(feature_info, "forest") || strstr(feature_info, "Forest")) {
                    feature->lithology = strdup("Forest");
                } else if (strstr(feature_info, "urban") || strstr(feature_info, "Urban")) {
                    feature->lithology = strdup("Urban");
                } else if (strstr(feature_info, "agricultural") || strstr(feature_info, "Agricultural")) {
                    feature->lithology = strdup("Agricultural");
                }
                
                printf("Feature %d: RGB(%d,%d,%d) at (%.6f, %.6f) -> %s\n",
                       i, feature->dominant_color.r, feature->dominant_color.g, feature->dominant_color.b,
                       cx, cy, feature->lithology ? feature->lithology : "Unknown");
            }
        }
    }
    
    // Write GeoJSON output
    char geojson_file[512];
    snprintf(geojson_file, sizeof(geojson_file), "%s.geojson", output_file);
    
    if (write_geojson(result, geojson_file) != 0) {
        fprintf(stderr, "Failed to write GeoJSON output\n");
        free_vectorization_result(result);
        return 1;
    }
    
    printf("Geological vectorization complete: %s\n", geojson_file);
    free_vectorization_result(result);
    return 0;
}

int vectorize_image(const char* input_file, const char* output_file) {
    printf("Vectorizing image: %s -> %s\n", input_file, output_file);
    
    // For now, create a simple text-based vector format
    char vector_file[512];
    snprintf(vector_file, sizeof(vector_file), "%s.vec", output_file);
    
    FILE* vec = fopen(vector_file, "w");
    if (!vec) {
        fprintf(stderr, "Failed to create vector file: %s\n", vector_file);
        return 1;
    }
    
    fprintf(vec, "# WMSPal Vector Output\n");
    fprintf(vec, "# Format: POLYGON((x1 y1, x2 y2, ...))\n");
    
#ifdef HAVE_GEOS
    fprintf(vec, "# Built with GEOS support for geometric operations\n");
    
    // Initialize GEOS
    initGEOS(NULL, NULL);
    
    // Create a simple polygon using GEOS
    GEOSCoordSequence* coords = GEOSCoordSeq_create(5, 2);
    GEOSCoordSeq_setX(coords, 0, 0.0);
    GEOSCoordSeq_setY(coords, 0, 0.0);
    GEOSCoordSeq_setX(coords, 1, 10.0);
    GEOSCoordSeq_setY(coords, 1, 0.0);
    GEOSCoordSeq_setX(coords, 2, 10.0);
    GEOSCoordSeq_setY(coords, 2, 10.0);
    GEOSCoordSeq_setX(coords, 3, 0.0);
    GEOSCoordSeq_setY(coords, 3, 10.0);
    GEOSCoordSeq_setX(coords, 4, 0.0);  // Close the ring
    GEOSCoordSeq_setY(coords, 4, 0.0);
    
    GEOSGeometry* ring = GEOSGeom_createLinearRing(coords);
    GEOSGeometry* polygon = GEOSGeom_createPolygon(ring, NULL, 0);
    
    char* wkt = GEOSGeomToWKT(polygon);
    if (wkt) {
        fprintf(vec, "%s\n", wkt);
        GEOSFree(wkt);
    }
    
    GEOSGeom_destroy(polygon);
    finishGEOS();
    
    printf("Vector file created with GEOS geometry: %s\n", vector_file);
#else
    fprintf(vec, "# Note: Full vectorization requires image processing library\n");
    fprintf(vec, "# This is a placeholder implementation\n");
    
    // Placeholder polygons
    fprintf(vec, "POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))\n");
    fprintf(vec, "POLYGON((20 20, 30 20, 30 30, 20 30, 20 20))\n");
    
    printf("Vector file created (basic): %s\n", vector_file);
#endif
    
    fclose(vec);
    printf("Note: For full image vectorization, consider using:\n");
    printf("  - OpenCV for C: https://opencv.org/\n");
    printf("  - GDAL for production use\n");
    printf("  - Or process with Python: scikit-image, opencv-python\n");
    
    return 0;
}

// GeoJSON output functions
int write_geojson(const vectorization_result_t* result, const char* output_file) {
    if (!result || !output_file) return 1;
    
    FILE* file = fopen(output_file, "w");
    if (!file) {
        fprintf(stderr, "Failed to create GeoJSON file: %s\n", output_file);
        return 1;
    }
    
    // Write GeoJSON header
    fprintf(file, "{\n");
    fprintf(file, "  \"type\": \"FeatureCollection\",\n");
    fprintf(file, "  \"crs\": {\n");
    fprintf(file, "    \"type\": \"name\",\n");
    fprintf(file, "    \"properties\": {\n");
    fprintf(file, "      \"name\": \"%s\"\n", result->crs);
    fprintf(file, "    }\n");
    fprintf(file, "  },\n");
    fprintf(file, "  \"bbox\": [%.6f, %.6f, %.6f, %.6f],\n", 
            result->minx, result->miny, result->maxx, result->maxy);
    fprintf(file, "  \"features\": [\n");
    
    // Write geological features
    for (int i = 0; i < result->feature_count; i++) {
        const geological_feature_t* feature = &result->features[i];
        
        if (i > 0) fprintf(file, ",\n");
        
        fprintf(file, "    {\n");
        fprintf(file, "      \"type\": \"Feature\",\n");
        fprintf(file, "      \"properties\": {\n");
        fprintf(file, "        \"feature_id\": %d,\n", i);
        fprintf(file, "        \"dominant_color\": \"rgb(%d,%d,%d)\",\n", 
                feature->dominant_color.r, feature->dominant_color.g, feature->dominant_color.b);
        
        if (feature->lithology) {
            fprintf(file, "        \"classification\": \"%s\",\n", feature->lithology);
        }
        if (feature->age) {
            fprintf(file, "        \"temporal_info\": \"%s\",\n", feature->age);
        }
        if (feature->geological_unit) {
            fprintf(file, "        \"unit_name\": \"%s\",\n", feature->geological_unit);
        }
        if (feature->feature_info) {
            // Escape quotes in feature info
            fprintf(file, "        \"wms_info\": \"");
            for (const char* p = feature->feature_info; *p; p++) {
                if (*p == '"') fprintf(file, "\\\"");
                else if (*p == '\n') fprintf(file, "\\n");
                else if (*p == '\r') fprintf(file, "\\r");
                else fprintf(file, "%c", *p);
            }
            fprintf(file, "\",\n");
        }
        
        fprintf(file, "        \"polygon_count\": %d\n", feature->polygon_count);
        fprintf(file, "      },\n");
        
        // Write geometry - MultiPolygon for multiple regions of same color
        fprintf(file, "      \"geometry\": {\n");
        if (feature->polygon_count == 1) {
            fprintf(file, "        \"type\": \"Polygon\",\n");
            fprintf(file, "        \"coordinates\": [[\n");
            
            const polygon_t* poly = &feature->polygons[0];
            for (int k = 0; k < poly->count; k++) {
                if (k > 0) fprintf(file, ",\n");
                fprintf(file, "          [%.8f, %.8f]", poly->coords[k].x, poly->coords[k].y);
            }
            // Close the polygon
            if (poly->count > 0) {
                fprintf(file, ",\n          [%.8f, %.8f]", poly->coords[0].x, poly->coords[0].y);
            }
            
            fprintf(file, "\n        ]]\n");
        } else {
            fprintf(file, "        \"type\": \"MultiPolygon\",\n");
            fprintf(file, "        \"coordinates\": [\n");
            
            for (int j = 0; j < feature->polygon_count; j++) {
                if (j > 0) fprintf(file, ",\n");
                fprintf(file, "          [[\n");
                
                const polygon_t* poly = &feature->polygons[j];
                for (int k = 0; k < poly->count; k++) {
                    if (k > 0) fprintf(file, ",\n");
                    fprintf(file, "            [%.8f, %.8f]", poly->coords[k].x, poly->coords[k].y);
                }
                // Close the polygon
                if (poly->count > 0) {
                    fprintf(file, ",\n            [%.8f, %.8f]", poly->coords[0].x, poly->coords[0].y);
                }
                
                fprintf(file, "\n          ]]");
            }
            
            fprintf(file, "\n        ]\n");
        }
        
        fprintf(file, "      }\n");
        fprintf(file, "    }");
    }
    
    // Close GeoJSON
    fprintf(file, "\n  ]\n");
    fprintf(file, "}\n");
    
    fclose(file);
    printf("GeoJSON written: %s (%d geological features)\n", output_file, result->feature_count);
    return 0;
}
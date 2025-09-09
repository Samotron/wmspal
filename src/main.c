#include "../include/wmspal.h"
#include <getopt.h>

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("WMS tile downloader and processor\n\n");
    printf("Options:\n");
    printf("  -u, --url URL         WMS service URL\n");
    printf("  -l, --layer LAYER     Layer name to download\n");
    printf("  -b, --bbox BBOX       Bounding box (minx,miny,maxx,maxy)\n");
    printf("  -s, --srs SRS         Spatial reference system (default: EPSG:4326)\n");
    printf("  -w, --width WIDTH     Image width in pixels (default: 256)\n");
    printf("  -h, --height HEIGHT   Image height in pixels (default: 256)\n");
    printf("  -f, --format FORMAT   Image format (default: image/png)\n");
    printf("  -o, --output FILE     Output file name\n");
    printf("  -v, --vectorize       Vectorize the georeferenced image\n");
    printf("      --vectorize-enhanced  Enhanced vectorization with color analysis and GetFeatureInfo\n");
    printf("      --vectorize-geological  Enhanced geological vectorization with GetFeatureInfo\n");
    printf("  -a, --attribution     Apply attribution using GetFeatureInfo\n");
    printf("  -c, --capabilities    Get WMS capabilities (requires --url)\n");
    printf("      --raw-xml         Show raw XML capabilities response\n");
    printf("      --help            Show this help message\n");
}

int main(int argc, char* argv[]) {
    wms_config_t config = {0};
    config.width = 256;
    config.height = 256;
    config.format = "image/png";
    config.srs = "EPSG:4326";
    
    static struct option long_options[] = {
        {"url", required_argument, 0, 'u'},
        {"layer", required_argument, 0, 'l'},
        {"bbox", required_argument, 0, 'b'},
        {"srs", required_argument, 0, 's'},
        {"width", required_argument, 0, 'w'},
        {"height", required_argument, 0, 'h'},
        {"format", required_argument, 0, 'f'},
        {"output", required_argument, 0, 'o'},
        {"vectorize", no_argument, 0, 'v'},
        {"attribution", no_argument, 0, 'a'},
        {"capabilities", no_argument, 0, 1001},
        {"vectorize-enhanced", no_argument, 0, 1004},
        {"vectorize-geological", no_argument, 0, 1003},
        {"raw-xml", no_argument, 0, 1002},
        {"help", no_argument, 0, 0},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "u:l:b:s:w:h:f:o:va", long_options, &option_index)) != -1) {
        switch (c) {
            case 'u':
                config.url = optarg;
                break;
            case 'l':
                config.layer = optarg;
                break;
            case 'b':
                config.bbox = optarg;
                break;
            case 's':
                config.srs = optarg;
                break;
            case 'w':
                config.width = atoi(optarg);
                break;
            case 'h':
                config.height = atoi(optarg);
                break;
            case 'f':
                config.format = optarg;
                break;
            case 'o':
                config.output_file = optarg;
                break;
            case 'v':
                config.vectorize = true;
                break;
            case 'a':
                config.attribution = true;
                break;
            case 1001:
                config.capabilities = true;
                break;
            case 1002:
                config.raw_xml = true;
                break;
            case 1003:
                config.vectorize_geological = true;
                break;
            case 1004:
                config.vectorize_enhanced = true;
                break;
            case 0:
                if (strcmp(long_options[option_index].name, "help") == 0) {
                    print_usage(argv[0]);
                    return 0;
                }
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    if (config.capabilities) {
        if (!config.url) {
            fprintf(stderr, "Error: URL is required for GetCapabilities\n");
            print_usage(argv[0]);
            return 1;
        }
        
        printf("Fetching WMS capabilities...\n");
        if (get_wms_capabilities(&config) != 0) {
            fprintf(stderr, "Error fetching WMS capabilities\n");
            return 1;
        }
        return 0;
    }
    
    if (!config.url || !config.layer || !config.bbox || !config.output_file) {
        fprintf(stderr, "Error: URL, layer, bbox, and output file are required\n");
        print_usage(argv[0]);
        return 1;
    }
    
    printf("Downloading WMS tile...\n");
    if (download_wms_tile(&config) != 0) {
        fprintf(stderr, "Error downloading WMS tile\n");
        return 1;
    }
    
    char georef_file[512];
    snprintf(georef_file, sizeof(georef_file), "%s_georef.tif", config.output_file);
    
    printf("Georeferencing image...\n");
    if (georeference_image(config.output_file, georef_file, config.bbox, config.srs) != 0) {
        fprintf(stderr, "Error georeferencing image\n");
        return 1;
    }
    
    if (config.vectorize || config.vectorize_enhanced || config.vectorize_geological) {
        char vector_file[512];
        snprintf(vector_file, sizeof(vector_file), "%s_vector.shp", config.output_file);
        
        if (config.vectorize_geological || config.vectorize_enhanced) {
            const char* workflow_type = config.vectorize_geological ? "geological" : "enhanced";
            printf("Enhanced %s vectorization...\n", workflow_type);
            if (vectorize_geological_map(georef_file, config.output_file, &config) != 0) {
                fprintf(stderr, "Error in %s vectorization\n", workflow_type);
                return 1;
            }
        } else {
            printf("Vectorizing image...\n");
            if (vectorize_image(georef_file, vector_file) != 0) {
                fprintf(stderr, "Error vectorizing image\n");
                return 1;
            }
        }
        
        if (config.attribution && !config.vectorize_geological && !config.vectorize_enhanced) {
            printf("Applying attribution...\n");
            if (apply_attribution(vector_file, &config) != 0) {
                fprintf(stderr, "Error applying attribution\n");
                return 1;
            }
        }
    }
    
    printf("Processing complete!\n");
    return 0;
}
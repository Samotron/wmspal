#include "../include/wmspal.h"
#include <curl/curl.h>

typedef struct {
    char* data;
    size_t size;
} response_buffer_t;

static size_t write_response_callback(void* contents, size_t size, size_t nmemb, response_buffer_t* buffer) {
    size_t realsize = size * nmemb;
    char* ptr = realloc(buffer->data, buffer->size + realsize + 1);
    
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    buffer->data = ptr;
    memcpy(&(buffer->data[buffer->size]), contents, realsize);
    buffer->size += realsize;
    buffer->data[buffer->size] = 0;
    
    return realsize;
}

int get_feature_info_at_point(const wms_config_t* config, double x, double y, char** result) {
    CURL* curl;
    CURLcode res;
    response_buffer_t response = {0};
    
    // Parse bbox to calculate pixel coordinates
    double minx, miny, maxx, maxy;
    if (sscanf(config->bbox, "%lf,%lf,%lf,%lf", &minx, &miny, &maxx, &maxy) != 4) {
        fprintf(stderr, "Invalid bbox format for GetFeatureInfo\n");
        return 1;
    }
    
    // Convert geographic coordinates to pixel coordinates
    int pixel_x = (int)((x - minx) / (maxx - minx) * config->width);
    int pixel_y = (int)((maxy - y) / (maxy - miny) * config->height);
    
    // Build GetFeatureInfo URL
    char url[2048];
    snprintf(url, sizeof(url), 
        "%s?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetFeatureInfo&LAYERS=%s&STYLES=&"
        "BBOX=%s&SRS=%s&WIDTH=%d&HEIGHT=%d&FORMAT=image/png&"
        "QUERY_LAYERS=%s&INFO_FORMAT=text/plain&X=%d&Y=%d",
        config->url, config->layer, config->bbox, config->srs, 
        config->width, config->height, config->layer, pixel_x, pixel_y);
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl for GetFeatureInfo\n");
        return 1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "WMSPal/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    printf("GetFeatureInfo query: (%.6f, %.6f) -> pixel (%d, %d)\n", x, y, pixel_x, pixel_y);
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "GetFeatureInfo request failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        if (response.data) free(response.data);
        return 1;
    }
    
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code != 200) {
        fprintf(stderr, "GetFeatureInfo HTTP error: %ld\n", response_code);
        curl_easy_cleanup(curl);
        if (response.data) free(response.data);
        return 1;
    }
    
    *result = response.data;  // Transfer ownership
    curl_easy_cleanup(curl);
    
    return 0;
}

int apply_attribution(const char* vector_file, const wms_config_t* config) {
    printf("Attribution functionality will query GetFeatureInfo for each vector feature\n");
    printf("Vector file: %s\n", vector_file);
    printf("WMS URL: %s\n", config->url);
    
    // This is now handled by the comprehensive geological vectorization workflow
    printf("Use --vectorize-geological for enhanced attribution with GetFeatureInfo\n");
    
    return 0;
}
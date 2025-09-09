#include "../include/wmspal.h"
#include <curl/curl.h>

typedef struct {
    char* data;
    size_t size;
} wms_response_t;

static size_t write_callback(void* contents, size_t size, size_t nmemb, wms_response_t* response) {
    size_t realsize = size * nmemb;
    char* ptr = realloc(response->data, response->size + realsize + 1);
    
    if (ptr == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    response->data = ptr;
    memcpy(&(response->data[response->size]), contents, realsize);
    response->size += realsize;
    response->data[response->size] = 0;
    
    return realsize;
}

static void parse_capabilities_simple(const char* xml) {
    printf("\n--- WMS Service Information ---\n");
    
    // Extract service title
    const char* title_start = strstr(xml, "<Title>");
    if (title_start) {
        title_start += 7;
        const char* title_end = strstr(title_start, "</Title>");
        if (title_end) {
            printf("Service Title: %.*s\n", (int)(title_end - title_start), title_start);
        }
    }
    
    // Extract service abstract
    const char* abstract_start = strstr(xml, "<Abstract>");
    if (abstract_start) {
        abstract_start += 10;
        const char* abstract_end = strstr(abstract_start, "</Abstract>");
        if (abstract_end) {
            printf("Abstract: %.*s\n", (int)(abstract_end - abstract_start), abstract_start);
        }
    }
    
    printf("\n--- Available Layers ---\n");
    
    // Find all layers
    const char* layer_pos = xml;
    int layer_count = 0;
    
    while ((layer_pos = strstr(layer_pos, "<Layer")) != NULL) {
        // Skip to the queryable attribute if present
        const char* queryable_start = strstr(layer_pos, "queryable=");
        bool queryable = false;
        if (queryable_start && queryable_start < strstr(layer_pos, ">")) {
            queryable_start += 11; // Skip 'queryable="'
            queryable = (strncmp(queryable_start, "1", 1) == 0);
        }
        
        // Find layer name
        const char* name_start = strstr(layer_pos, "<Name>");
        if (name_start) {
            name_start += 6;
            const char* name_end = strstr(name_start, "</Name>");
            if (name_end) {
                printf("Layer %d: %.*s", ++layer_count, (int)(name_end - name_start), name_start);
                if (queryable) printf(" (queryable)");
                printf("\n");
                
                // Find layer title
                const char* layer_title_start = strstr(name_start, "<Title>");
                if (layer_title_start && layer_title_start < strstr(name_start, "</Layer>")) {
                    layer_title_start += 7;
                    const char* layer_title_end = strstr(layer_title_start, "</Title>");
                    if (layer_title_end) {
                        printf("  Title: %.*s\n", (int)(layer_title_end - layer_title_start), layer_title_start);
                    }
                }
            }
        }
        
        layer_pos++;
    }
    
    if (layer_count == 0) {
        printf("No layers found in capabilities response.\n");
    }
    
    printf("\n--- Supported Formats ---\n");
    
    // Find supported formats
    const char* format_pos = xml;
    while ((format_pos = strstr(format_pos, "<Format>")) != NULL) {
        format_pos += 8;
        const char* format_end = strstr(format_pos, "</Format>");
        if (format_end) {
            printf("Format: %.*s\n", (int)(format_end - format_pos), format_pos);
        }
        format_pos = format_end ? format_end : format_pos + 1;
    }
}

int get_wms_capabilities(const wms_config_t* config) {
    CURL* curl;
    CURLcode res;
    wms_response_t response = {0};
    
    char url[2048];
    snprintf(url, sizeof(url), 
        "%s?SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities",
        config->url);
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "WMSPal/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    printf("Fetching capabilities: %s\n", url);
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        if (response.data) free(response.data);
        return 1;
    }
    
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code != 200) {
        fprintf(stderr, "HTTP error: %ld\n", response_code);
        curl_easy_cleanup(curl);
        if (response.data) free(response.data);
        return 1;
    }
    
    printf("\n--- WMS Capabilities ---\n");
    if (config->raw_xml) {
        printf("%s\n", response.data);
    } else {
        parse_capabilities_simple(response.data);
    }
    
    curl_easy_cleanup(curl);
    if (response.data) free(response.data);
    
    return 0;
}

int download_wms_tile(const wms_config_t* config) {
    CURL* curl;
    CURLcode res;
    wms_response_t response = {0};
    
    char url[2048];
    snprintf(url, sizeof(url), 
        "%s?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&LAYERS=%s&STYLES=&BBOX=%s&SRS=%s&WIDTH=%d&HEIGHT=%d&FORMAT=%s",
        config->url, config->layer, config->bbox, config->srs, config->width, config->height, config->format);
    
    curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to initialize curl\n");
        return 1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "WMSPal/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    printf("Downloading: %s\n", url);
    res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        if (response.data) free(response.data);
        return 1;
    }
    
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code != 200) {
        fprintf(stderr, "HTTP error: %ld\n", response_code);
        curl_easy_cleanup(curl);
        if (response.data) free(response.data);
        return 1;
    }
    
    FILE* file = fopen(config->output_file, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open output file: %s\n", config->output_file);
        curl_easy_cleanup(curl);
        if (response.data) free(response.data);
        return 1;
    }
    
    fwrite(response.data, 1, response.size, file);
    fclose(file);
    
    printf("Downloaded %zu bytes to %s\n", response.size, config->output_file);
    
    curl_easy_cleanup(curl);
    if (response.data) free(response.data);
    
    return 0;
}
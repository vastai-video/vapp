#include "op_case.h"


char** split_string(char* str, const char* delimiter, int* file_count) {
    char** splits = NULL;
    char* token = strtok(str, delimiter);
    int count = 0;

    while (token != NULL) {
        splits = realloc(splits, sizeof(char*) * (count + 1));
        if (splits == NULL) {
            printf("Memory allocation failed\n");
            return NULL;
        }
        splits[count] = token;
        count++;
        token = strtok(NULL, delimiter);
    }

    *file_count = count;   

    return splits;
}

   
void print_help() {
    printf("Usage: program_name --inputfile <input_file> --outputfile <output_file> --input_size <widthxheight> --elf_file <elf_file> --op_name <operation_name>"
    "--pixel_format <rgb24, yuv420>  --test_case <0,1...> --vframes\n");
    printf("case list : \n");
    printf("   case 0 : rgb888 resize  \n");
    printf("   case 1 : rgb888 cvtcolor\n");

}

int parse_command_line(int argc, char *argv[], CommandLineArgs *args) {
    int opt;
    int option_index = 0;
    char *p = NULL;

    static struct option long_options[] = {
        {"inputfile", required_argument, 0, 'i'},
        {"outputfile", required_argument, 0, 'o'},
        {"input_size", required_argument, 0, 's'},
        {"input_pitch", required_argument, 0, 0},
        {"layer_size", required_argument, 0, 0},
        {"output_size", required_argument, 0, 'r'},
        {"output_pitch", required_argument, 0, 0},
        {"pixel_format", required_argument, 0, 'p'},
        {"test_case", required_argument, 0, 'c'},
        {"vframes", required_argument, 0, 'v'},
        {"config_file", required_argument, 0, 0},
        {"brightness", required_argument, 0, 0},
        {"contrast", required_argument, 0, 0},
        {"saturation", required_argument, 0, 0},
        {"device_id", required_argument, 0, 'd'},
        {"degree", required_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    args->contrast = 1.0;
    args->saturation = 1.0;
    args->brightness = 0;
    args->device_id = 0;
    args->plane = 1;

    while ((opt = getopt_long(argc, argv, "i:o:s:r:p:c:v:h:d", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'i':
                args->input_file = split_string(optarg, " ", &args->input_count);
                break;
            case 'o':
                args->output_file = optarg;
                break;
            case 's':
                args->input_size.width = strtol(optarg, (void*)&p, 10);
                if (*p)
                    p++;
                args->input_size.height = strtol(p, (void*)&p, 10);
                break;
            case 'r':
                args->output_size.width = strtol(optarg, (void*)&p, 10);
                if (*p)
                    p++;
                args->output_size.height = strtol(p, (void*)&p, 10);
                break;                
            case 'p':
                if(strstr(optarg, "rgba") || strstr(optarg, "bgra")){
                    args->plane = 4;
                }else if (strstr(optarg, "rgb") || strstr(optarg, "bgr")){
                    args->plane = 3;
                }else{
                    args->plane = 1;
                }
                break;    
            case 'c':
                args->test_case = atoi(optarg);
                break;       
            case 'v':
                args->frame_count = atoi(optarg);
                break;    
            case 'd':
                args->device_id =  atoi(optarg);  
                break;                            
            case 'h':
                print_help();
                return 0;
            case '?':
                print_help();
                return 1;
            case 0:
                if (optarg && strcmp(long_options[option_index].name, "layer_size") == 0){
                    args->layer_width = strtol(optarg, (void*)&p, 10);
                    if (*p)
                        p++;
                    args->layer_height = strtol(p, (void*)&p, 10);                    
                }  
                else if(optarg && strcmp(long_options[option_index].name, "input_pitch") == 0){
                     args->input_size.wPitch = strtol(optarg, (void*)&p, 10);
                     if (*p)
                        p++;
                     args->input_size.hPitch = strtol(p, (void*)&p, 10); 
                }     
                else if(optarg && strcmp(long_options[option_index].name, "output_pitch") == 0){
                    args->output_size.wPitch = strtol(optarg, (void*)&p, 10);
                    if (*p)
                        p++;
                    args->output_size.hPitch = strtol(p, (void*)&p, 10); 
                }         
                else if(optarg && strcmp(long_options[option_index].name, "config_file") == 0){
                    args->config_file = optarg;
                }                   
                else if  (optarg && strcmp(long_options[option_index].name, "brightness") == 0){
                    args->brightness = strtod(optarg, (void*)&p);
                }
                else if  (optarg && strcmp(long_options[option_index].name, "contrast") == 0){
                    args->contrast = strtod(optarg, (void*)&p);
                }
                else if  (optarg && strcmp(long_options[option_index].name, "saturation") == 0){
                    args->saturation = strtod(optarg, (void*)&p);
                }
                else if  (optarg && strcmp(long_options[option_index].name, "degree") == 0){
                    args->degree = strtod(optarg, (void*)&p);
                }
                

                break;         
            default:
                // Check for long options
                if (long_options[option_index].flag != 0) {                  
                    break; // skip if it's a recognized long option
                }
                print_help();
                return 1;
        }
    }

    // Check if required options are provided
    if (args->input_file == NULL || args->output_file == NULL || args->input_size.width <= 0 || args->input_size.height <= 0) {
        printf("Error: Missing or invalid parameters.\n");
        print_help();
        return 1;
    }

    return 0;
}


void test_remap_threshold_multistream2(int stream_num);
int main(int argc, char *argv[]) 
{
#if 1
   CommandLineArgs args = {0};
   if (parse_command_line(argc, argv, &args) != 0) {
       fprintf(stderr, "parse_command_line failed.\n");
       return 1;
   }
   run_test_case(&args);                         
#else 
    int stream_num = 1;
    int device_id = 0;
    if (argc > 1) {
        stream_num = atoi(argv[1]);
    }


    test_remap_threshold_multistream2(stream_num);
#endif
   return 0;
}



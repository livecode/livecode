#include "graphics.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: rendersvg <inputsvg> <outputimg>\n");
        return 1;
    }
    
    const char *t_input_filename;
    t_input_filename = argv[1];
    
    const char *t_output_filename;
    t_output_filename = argv[2];
    
    ////
    
    FILE *fp;
    fp = fopen(t_input_filename, "rb");
	if (fp == NULL)
    {
        fprintf(stderr, "Couldn't open input file '%s'\n", t_input_filename);
        return 1;
    }
	fseek(fp, 0, SEEK_END);
	size_t t_size;
    t_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
    char *t_data;
	t_data = (char*)malloc(t_size);
	if (t_data == NULL)
        return -1;
	fread(t_data, t_size, 1, fp);
    
    MCGSvgRef t_svg;
    MCGSvgCreate(t_data, t_size, t_svg);
    
    free(t_data);
    
    MCGContextRef t_gcontext;
    MCGContextCreate(400, 400, true, t_gcontext);
    
    MCGRectangle t_bbox;
    t_bbox = MCGSvgGetBoundingBox(t_svg);
    
    MCGContextScaleCTM(t_gcontext,
                       400.0f / t_bbox . size . width,
                       400.0f / t_bbox . size . height);
    
    MCGSvgRender(t_svg, t_gcontext);
    
    MCGContextRelease(t_gcontext);
    
    MCGSvgRelease(t_svg);
    
    return 0;
}

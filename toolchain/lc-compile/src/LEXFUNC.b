static FILE *s_current_stream = NULL;
void yynextfile(FILE *stream)
{
    yy_delete_buffer(YY_CURRENT_BUFFER);

    if (s_current_stream != NULL)
        fclose(s_current_stream);
        
    s_current_stream = stream;

    yy_switch_to_buffer(yy_create_buffer(stream, YY_BUF_SIZE));
}

void yynextfile(FILE *stream)
{
    yy_delete_buffer(YY_CURRENT_BUFFER);
    yy_switch_to_buffer(yy_create_buffer(stream, YY_BUF_SIZE));
}

# Create a library called "Hello" which includes the source file "hello.cxx".
# The extension is already found. Any number of sources could be listed here.

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_CONFIG_H=1")

list(APPEND speex_sources
    bits.c              
    kiss_fftr.c
    cb_search.c         
    lpc.c
    exc_10_16_table.c   
    exc_10_32_table.c   
    exc_20_32_table.c   
    exc_5_256_table.c   
    exc_5_64_table.c    
    exc_8_128_table.c   
    fftwrap.c           
    filterbank.c        
    filters.c           
    gain_table.c        
    gain_table_lbr.c    
    hexc_10_32_table.c  
    hexc_table.c        
    high_lsp_tables.c   
    kiss_fft.c
    lsp.c
    lsp_tables_nb.c 
    ltp.c
    mdf.c
    modes.c
    nb_celp.c
    preprocess.c
    quant_lsp.c
    speex.c
    speex_callbacks.c
    vq.c
    window.c
    resample.c)
#    vbr.c
#    sb_celp.c)


add_library (speex ${speex_sources} )

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (speex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})



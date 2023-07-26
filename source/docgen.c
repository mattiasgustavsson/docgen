#define FILE_IMPLEMENTATION
#include "file.h"

#define HOEDOWN_IMPLEMENTATION
#include "hoedown.h"

#include <stdbool.h>
#include <string.h>

#ifndef _WIN32
    #include <strings.h>
    #define strnicmp( s1, s2, len ) ( strncasecmp( (s1), (s2), (len) ) )
#endif

size_t remove_atlines( void* in_out, size_t len ) {
    char const* src = (char const*) in_out;
    char const* dst = (char const*) in_out;

    char const* ptr = (char const*) src;
	char* dst_ptr = (char*) dst;
    bool newline = true;
    bool skip_until_newline = false;
	while( (size_t)( ptr - (char const*) src ) < len ) {
        if( skip_until_newline ) {
            if( *ptr == '\n' ) {
                skip_until_newline = false;
                newline = true;
            }
            ptr++;
        } else if( newline ) {
            newline = false;
            if( *ptr == '@' ) {
                skip_until_newline = true;
                ptr++;
            } else if( *ptr == '\n' ) {
                newline = true;
                *dst_ptr++ = *ptr++;
            }
        } else {
            if( *ptr == '\n' ) newline = true;
            *dst_ptr++ = *ptr++;
        }
    }
    return (size_t)( dst_ptr - (char*) dst );
}


size_t remove_attags( void* in_out, size_t len ) {
    char const* src = (char const*) in_out;
    char const* dst = (char const*) in_out;

    char const* ptr = (char const*) src;
	char* dst_ptr = (char*) dst;
    bool skip_until_whitespace = false;
	while( (size_t)( ptr - (char const*) src ) < len ) {
        if( skip_until_whitespace ) {
            if( *ptr <= ' ' ) {
                skip_until_whitespace = false;
            }
            ptr++;
        } else if( *ptr == '@' ) {
            skip_until_whitespace = true;
            ptr++;
        } else {
            *dst_ptr++ = *ptr++;
        }
    }
    return (size_t)( dst_ptr - (char*) dst );
}


size_t copy_insert_divs( void* dst, void const* src, size_t len ) {
	char const* ptr = (char const*) src;
	char* dst_ptr = (char*) dst;
	bool in_div[ 6 ] = { false, false, false, false, false, false };
	size_t added_size = 0;
	while( (size_t)( ptr - (char const*) src ) < len ) {
		if( strnicmp( ptr, "<div>", 5 ) == 0 || strnicmp( ptr, "<div ", 5 ) == 0) {
			for( int i = 0; i < 6; ++i ) {
				if( in_div[ i ] ) {
					memcpy( dst_ptr, "</div>\n\n", 8 );
					dst_ptr += 8;
					added_size += 8;
					in_div[ i ] = false;
				}
			}
			memcpy( dst_ptr, ptr, 5 ); 
			dst_ptr += 5;
			ptr += 5;
		} else if( strnicmp( ptr, "<h1>", 4 ) == 0 || 
			strnicmp( ptr, "<h2>", 4 ) == 0 ||
			strnicmp( ptr, "<h3>", 4 ) == 0 ||
			strnicmp( ptr, "<h4>", 4 ) == 0 ||
			strnicmp( ptr, "<h5>", 4 ) == 0 ||
			strnicmp( ptr, "<h6>", 4 ) == 0 ) {

			if( in_div[ ptr[ 2 ] - '1' ] ) {
				memcpy( dst_ptr, "</div>\n\n", 8 );
				dst_ptr += 8;
				added_size += 8;
				in_div[ ptr[ 2 ] - '1' ] = false;
			}
			memcpy( dst_ptr, ptr, 4 ); 
			dst_ptr += 4;
			ptr += 4;
			continue;
		} else if( strnicmp( ptr, "</h1>", 5 ) == 0 || 
			strnicmp( ptr, "</h2>", 5 ) == 0 ||
			strnicmp( ptr, "</h3>", 5 ) == 0 ||
			strnicmp( ptr, "</h4>", 5 ) == 0 ||
			strnicmp( ptr, "</h5>", 5 ) == 0 ||
			strnicmp( ptr, "</h6>", 5 ) == 0 ) {

			memcpy( dst_ptr, ptr, 5 ); 
			dst_ptr += 5;
			in_div[ ptr[ 3 ] - '1' ] = true;
			char div_tag[] = "\n<div class=\"hx\">";
			div_tag[ 14 ] = ptr[ 3 ];
			memcpy( dst_ptr, div_tag, 17 );
			dst_ptr += 17;
			added_size += 17;
			ptr += 5;
			continue;
		}
		*dst_ptr++ = *ptr;
		++ptr;
	}
	for( int i = 0; i < 6; ++i ) {
		if( in_div[ i ] ) {
			memcpy( dst_ptr, "</div>\n\n", 8 );
			dst_ptr += 8;
			added_size += 8;
			in_div[ i ] = false;
		}
	}
	return added_size;
}


void print_help( void ) {
    printf( "\ndocgen 0.1 - by Mattias Gustavsson\n\n");
    printf( "Examples:\n" );
    printf( "\t docgen -begin-tag /** -end-tag */ -strip-at-lines -strip-at-tags myfile.h myfile.html\n\n" );
    printf( "\t docgen myfile.md myfile.html\n\n" );
}


int main( int argc, char **argv ) {
    bool strip_atlines = false;
    bool strip_attags = false;
    char const* input_filename = 0;
    char const* output_filename = 0;
    char const* markdown_begin_tag = "<DOC";
    char const* markdown_end_tag = "DOC>";
	bool whole_file = true;

    for( int i = 1; i < argc; ++i ) {
        char const* arg = argv[ i ];
		if( strcmp( arg, "-file" ) == 0 ) {
			whole_file = true;
		} else if( strcmp( arg, "-begin-tag" ) == 0 ) {
            if( i++ >= argc ) {
                print_help();
                return EXIT_FAILURE;
            }
			whole_file = false;
            markdown_begin_tag = argv[ i ];
        } else if( strcmp( arg, "-end-tag" ) == 0 ) {
            if( i++ >= argc ) {
                print_help();
                return EXIT_FAILURE;
            }
			whole_file = false;
            markdown_end_tag = argv[ i ];
        } else if( strcmp( arg, "-strip-at-lines" ) == 0 ) {
            strip_atlines = true;
        } else if( strcmp( arg, "-strip-at-tags" ) == 0 ) {
            strip_attags = true;
        } else if( *arg != '-' ) {
            if( !input_filename ) { 
                input_filename = argv[ i ];
			} else if( !output_filename ) {
                output_filename = argv[ i ];
			} else {
                print_help();
                return EXIT_FAILURE;
            }
        } else {
            print_help();
            return EXIT_FAILURE;
        }
    }

    if( !input_filename ) {
        print_help();
        return EXIT_FAILURE;
    }

    char output_filename_buf[ 260 ];
    if( !output_filename ) {
        strcpy( output_filename_buf, input_filename );
        char* ext = strrchr( output_filename_buf, '.' );
        if( !ext ) {
            strcat( output_filename_buf, ".html" );
		} else {
            strcpy( ext, ".html" );
		}
        output_filename = output_filename_buf;
    }

    size_t markdown_begin_tag_len = strlen( markdown_begin_tag );
    size_t markdown_end_tag_len = strlen( markdown_end_tag );

    file_t* file = file_load( input_filename, FILE_MODE_TEXT, NULL );
    if( !file ) {
        print_help();
        return EXIT_FAILURE;
    }
	char* markdown_source = (char*) malloc( file->size );
	char* markdown_source_end = markdown_source;
	if( whole_file ) {
		memcpy( markdown_source, file->data, file->size );
		markdown_source_end = markdown_source + file->size;
	} else {
		char const* ptr = file->data;
		while( *ptr ) {
			if( strnicmp( ptr, markdown_begin_tag, markdown_begin_tag_len ) == 0 ) {
				ptr += markdown_begin_tag_len;
				char const* start = ptr;
				char const* end = 0;
				while( *ptr ) {
					if( strnicmp( ptr, markdown_end_tag, markdown_end_tag_len ) == 0 ) {
						end = ptr;
						ptr += markdown_end_tag_len;
						break;
					}
					++ptr;
				}				
				if( end ) {
					size_t length = end - start;
					strncpy( markdown_source_end, start, length );
					markdown_source_end += length;				
					continue;
				}
			}
			++ptr;
		}
		*markdown_source_end = 0;

		if( strip_atlines ) {
			size_t new_len = remove_atlines( markdown_source, markdown_source_end - markdown_source );
			markdown_source_end = markdown_source + new_len; 
			*markdown_source_end = 0;
		}

		if( strip_attags ) {
			size_t new_len = remove_attags( markdown_source, markdown_source_end - markdown_source );
			markdown_source_end = markdown_source + new_len; 
			*markdown_source_end = 0;
		}
	}
	
	file_destroy( file );

	hoedown_renderer* renderer = hoedown_html_renderer_new( 
		(hoedown_html_flags) ( 0
		//| HOEDOWN_HTML_SKIP_HTML // Strip all HTML tags.
		| HOEDOWN_HTML_ESCAPE // Escape all HTML.
		//| HOEDOWN_HTML_HARD_WRAP // Render each linebreak as <br>.
		| HOEDOWN_HTML_USE_XHTML // Render XHTML.
		), 0 );

	hoedown_buffer* ob_doc = hoedown_buffer_new( 1024 );
	hoedown_document* document = hoedown_document_new( renderer, 
		(hoedown_extensions) ( 0
		| HOEDOWN_EXT_TABLES // Parse PHP-Markdown style tables.
		| HOEDOWN_EXT_FENCED_CODE // Parse fenced code blocks.
		| HOEDOWN_EXT_FOOTNOTES // Parse footnotes.
		| HOEDOWN_EXT_AUTOLINK // Automatically turn safe URLs into links.
		| HOEDOWN_EXT_STRIKETHROUGH // Parse ~~strikethrough~~ spans.
		| HOEDOWN_EXT_UNDERLINE // Parse _underline_ instead of emphasis.
		| HOEDOWN_EXT_HIGHLIGHT // Parse ==highlight== spans.
		//| HOEDOWN_EXT_QUOTE // Render "quotes" as <q>quotes</q>.
		//| HOEDOWN_EXT_SUPERSCRIPT // Parse super^script.
		| HOEDOWN_EXT_MATH // Parse TeX $$math$$ syntax, Kramdown style.
		| HOEDOWN_EXT_NO_INTRA_EMPHASIS // Disable emphasis_between_words.
		| HOEDOWN_EXT_SPACE_HEADERS // Require a space after '#' in headers.
		//| HOEDOWN_EXT_MATH_EXPLICIT // Instead of guessing by context, parse $inline math$ and $$always block math$$ (requires --math).
		//| HOEDOWN_EXT_DISABLE_INDENTED_CODE // Don't parse indented code blocks.
		), 64 );

	hoedown_document_render( document, ob_doc, (uint8_t*) markdown_source, markdown_source_end - markdown_source );
	free( markdown_source );
	hoedown_document_free( document );
	hoedown_html_renderer_free( renderer );

	hoedown_buffer* ob_sp = hoedown_buffer_new( 1024 );
	hoedown_html_smartypants( ob_sp, ob_doc->data, ob_doc->size);
	hoedown_buffer_free( ob_doc );

	char const* default_html_header = 
		"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
		"<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
		"<head>\n"
		"<link rel=\"stylesheet\" type=\"text/css\" href=\"style.css\">\n"
		"</head>\n"
		"<body>\n";

	char const* default_html_footer = 
		"</body>\n"
		"</html>\n";

	char const* html_header = default_html_header;
	char const* html_footer = default_html_footer;
	size_t html_header_size = strlen( html_header );
	size_t html_footer_size = strlen( html_footer );
	bool div_after_heading = false;

	size_t html_size = ob_sp->size + html_header_size + html_footer_size;
	char* html_document = (char*) malloc( 1 + html_size + ( div_after_heading ? ( html_size * 30 ) / 9 : 0 ) );
	char* html_ptr = html_document;
	memcpy( html_ptr, html_header, html_header_size );
	html_ptr += html_header_size;
	if( div_after_heading ) {
		size_t added_size = copy_insert_divs( html_ptr, ob_sp->data, ob_sp->size );
		html_ptr += ob_sp->size + added_size;
		html_size += added_size;
	} else {
		memcpy( html_ptr, ob_sp->data, ob_sp->size );
		html_ptr += ob_sp->size;
	}
	hoedown_buffer_free( ob_sp );
	memcpy( html_ptr, html_footer, html_footer_size );
	html_ptr += html_footer_size;
	*html_ptr = 0;

	file_save_data( html_document, html_size + 1, output_filename, FILE_MODE_TEXT );	

	return EXIT_SUCCESS;
}


/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2016 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/

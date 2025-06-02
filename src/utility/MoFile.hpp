#pragma once
/*

The MIT License

Copyright (c) 2007 Jonathan Blow (jon [at] number-none [dot] com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

/*
 This code is designed to be a very simple drop-in method for reading .mo
 files, which are a standard format for storing text strings to help with
 internationalization.

 For more information, see: http://www.gnu.org/software/gettext/manual/gettext.html

 Unfortunately the gettext code is unwieldy.  For a simple program like
 a video game, we just want to look up some strings in a pre-baked file,
 which is what this code does.

 We assume that you started with a .po file (which is a human-readable format
 containing all the strings) and then compiled it into a .mo file (which is
 a binary format that can be efficiently read into memory all in one chunk).
 This code then reads straight from the .mo file, so there is no extra
 string allocation and corresponding memory fragmentation.

 You can generate a .mo file from a .po file using programs such as poedit
 or msgfmt.

 This code assumes you compiled the hash table into the .mo file.  It also
 doesn't attempt to care about the encoding of your text; it assumes you
 already know what that is.  (I use UTF-8 which seems like the only sane
 choice).

 There's no good reason that this is a C++ file, rather than a C file; I just
 wrote it that way originally.  You can trivially get rid of the C++-ness if
 you want it to compile as straight C.

 Send questions to jon [at] number-none [dot] com.

 21 December 2007
*/

// From the header file:
namespace utility {

struct LocalizationText {
    explicit LocalizationText(void* _mo_data);
    ~LocalizationText();

    void abort();

    void* mo_data;
    int reversed;

    int num_strings;
    int original_table_offset;
    int translated_table_offset;
    int hash_num_entries;
    int hash_offset;
};

char* text_lookup(char* s);
void mo_load(void* mo_data);

struct ImGooListboxTranslated {

    ImGooListboxTranslated(ImGooListboxTranslated const &) = delete;
    void operator=(ImGooListboxTranslated const &l) = delete;

    explicit ImGooListboxTranslated(const char* const items[], int items_count);
    ~ImGooListboxTranslated();

    inline const char* const* data() const {
        return data_;
    }

    inline const int size() const {
        return count;
    }

public:
    char** data_;
    int count;
};

} // namespace utility
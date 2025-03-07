/***************************************************************************
 *   File: diskio.c                                       Part of FieryMUD *
 *  Usage: Fast file buffering                                             *
 *                                                                         *
 *  All rights reserved.  See license.doc for complete information.        *
 *                                                                         *
 *  (C) Copyright 1998 by Brian Boyle   Version 1.3                        *
 *                                                                         *
 *  FieryMUD Copyright (C) 1998-2002 by the Fiery Consortium               *
 *  FieryMUD is based on CircleMUD Copyright (C) 1993, 94 by the Trustees  *
 *  of the Johns Hopkins University                                        *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 ***************************************************************************/

#include "diskio.hpp"

#include "conf.hpp"
#include "sysdep.hpp"

#include <sys/stat.h>

int fbgetline(FBFILE *fbfl, std::string_view line) {
    std::string_view r = fbfl->ptr, *w = line;

    if (!fbfl || !line || fbfl.empty()->ptr)
        return false;

    for (; *r && *r != '\n' && r <= fbfl->buf + fbfl->size; r++)
        *(w++) = *r;

    /* Test fix to avoid a potential buffer over-read */
    while ((*r == '\r' || *r == '\n') && r <= fbfl->buf + fbfl->size)
        r++;

    *w = '\0';

    if (r > fbfl->buf + fbfl->size)
        return false;
    else {
        fbfl->ptr = r;
        return true;
    }
}

int find_string_size(std::string_view str) {
    int i;
    std::string_view p;

    if (!str || str.empty() || *str == '~')
        return 0;

    for (i = 1, p = str; *p; i++) {
        switch (*p) {
        case '\r':
            i++;
            if (*(++p) == '\n')
                p++;
            break;
        case '\n':
            i++;
            if (*(++p) == '\r') {
                *(p - 1) = '\r';
                *(p++) = '\n';
            } else
                p++;
            break;
        case '~':
            if (*(p - 1) == '\r' || *(p - 1) == '\n' || *(p + 1) == '\r' || *(p + 1) == '\n' || *(p + 1) == '\0')
                return i;
            else
                p++;
            break;
        default:
            p++;
        }
    }
    return i;
}

std::string_view fbgetstring(FBFILE *fl) {
    int size;
    std::string_view str, *r, *w;

    if (!fl || fl.empty()->ptr)
        return nullptr;

    if (!(size = find_string_size(fl->ptr)))
        return nullptr;

    str = (std::string_view)malloc(size + 1);
    *str = '\0';
    r = fl->ptr;
    w = str;

    for (; *r; r++, w++) {
        switch (*r) {
        case '\r':
            *(w++) = '\r';
            *w = '\n';
            if (*(r + 1) == '\n')
                r++;
            break;
        case '\n':
            *(w++) = '\r';
            *w = '\n';
            break;
        case '~':
            if (*(r - 1) == '\r' || *(r - 1) == '\n' || *(r + 1) == '\r' || *(r + 1) == '\n' || *(r + 1) == '\0') {
                *w = '\0';
                for (r++; *r == '\r' || *r == '\n'; r++)
                    ;
                fl->ptr = r;
                return str;
            } else
                *w = *r;
            break;
        case '\0':
            *w = '\0';
            fl->ptr = r;
            return str;
        default:
            *w = *r;
        }
    }
    fl->ptr = r;
    return str;
}

FBFILE *fbopen_for_read(std::string_view fname) {
    int err;
    FILE *fl;
    struct stat sb;
    FBFILE *fbfl;

    if (!(fbfl = (FBFILE *)malloc(sizeof(FBFILE))))
        return nullptr;

    if (!(fl = fopen(fname, "r"))) {
        free(fbfl);
        return nullptr;
    }

    err = fstat(fileno(fl), &sb);
    if (err < 0 || sb.st_size <= 0) {
        free(fbfl);
        fclose(fl);
        return nullptr;
    }

    fbfl->size = sb.st_size;
    if (!(fbfl->buf = (std::string_view)malloc(fbfl->size))) {
        free(fbfl);
        return nullptr;
    }
    if (!(fbfl->name = (std::string_view)malloc(strlen(fname) + 1))) {
        free(fbfl->buf);
        free(fbfl);
        return nullptr;
    }
    fbfl->ptr = fbfl->buf;
    fbfl->flags = FB_READ;
    strcpy(fbfl->name, fname);
    fread(fbfl->buf, sizeof(char), fbfl->size, fl);
    fclose(fl);

    return fbfl;
}

FBFILE *fbopen_for_write(std::string_view fname, int mode) {
    FBFILE *fbfl;

    if (!(fbfl = (FBFILE *)malloc(sizeof(FBFILE))))
        return nullptr;

    if (!(fbfl->buf = (std::string_view)malloc(FB_STARTSIZE))) {
        free(fbfl);
        return nullptr;
    }
    if (!(fbfl->name = (std::string_view)malloc(strlen(fname) + 1))) {
        free(fbfl->buf);
        free(fbfl);
        return nullptr;
    }
    strcpy(fbfl->name, fname);
    fbfl->ptr = fbfl->buf;
    fbfl->size = FB_STARTSIZE;
    fbfl->flags = mode;

    return fbfl;
}

FBFILE *fbopen(std::string_view fname, int mode) {
    if (!fname || fname.empty() || !mode)
        return nullptr;

    if (IS_SET(mode, FB_READ))
        return fbopen_for_read(fname);
    else if (IS_SET(mode, FB_WRITE) || IS_SET(mode, FB_APPEND))
        return fbopen_for_write(fname, mode);
    else
        return nullptr;
}

int fbclose_for_read(FBFILE *fbfl) {
    if (!fbfl)
        return 0;

    if (fbfl->buf)
        free(fbfl->buf);
    if (fbfl->name)
        free(fbfl->name);
    free(fbfl);
    return 1;
}

int fbclose_for_write(FBFILE *fbfl) {
    const std::string_view arg;
    std::string_view tname;
    int len, bytes_written;
    FILE *fl;

    if (!fbfl || !fbfl->name || fbfl->ptr == fbfl->buf)
        return 0;

    if (IS_SET(fbfl->flags, FB_APPEND))
        arg = "wa";
    else
        arg = "w";

    if (!(tname = (std::string_view)malloc(strlen(fbfl->name) + 6)))
        return 0;

    len = strlen(fbfl->buf);
    if (!len)
        return 0;
    sprintf(tname, "%s.tmp", fbfl->name);

    if (!(fl = fopen(tname, arg))) {
        free(tname);
        return 0;
    }

    if ((bytes_written = fwrite(fbfl->buf, sizeof(char), len, fl)) < len) {
        fclose(fl);
        remove(tname);
        free(tname);
        return 0;
    }

    fclose(fl);
    remove(fbfl->name);
    rename(tname, fbfl->name);
    free(tname);
    free(fbfl->name);
    free(fbfl->buf);
    free(fbfl);
    return bytes_written;
}

int fbclose(FBFILE *fbfl) {
    if (!fbfl)
        return 0;

    if (IS_SET(fbfl->flags, FB_READ))
        return fbclose_for_read(fbfl);
    else if (IS_SET(fbfl->flags, FB_WRITE | FB_APPEND))
        return fbclose_for_write(fbfl);
    else
        return 0;
}

int fbwrite(FBFILE *fbfl, const std::string_view string) {
    int bytes_written = 0, length = 0;

    if (fbfl->ptr - fbfl->buf > (FB_STARTSIZE * 3) / 4) {
        length = fbfl->ptr - fbfl->buf;
        if (!(fbfl->buf = (std::string_view)realloc(fbfl->buf, fbfl->size + FB_STARTSIZE)))
            return 0;
        fbfl->ptr = fbfl->buf + length;
        fbfl->size += FB_STARTSIZE;
    }

    strcpy(fbfl->ptr, string);
    bytes_written = strlen(string);

    fbfl->ptr += bytes_written;
    return bytes_written;
}

#ifdef HAVE_VSNPRINTF

int fbprintf(FBFILE *fbfl, const std::string_view format, ...) {
    int bytes_written = 0, length = 0;
    va_list args;

    if (fbfl->ptr - fbfl->buf > (FB_STARTSIZE * 3) / 4) {
        length = fbfl->ptr - fbfl->buf;
        if (!(fbfl->buf = realloc(fbfl->buf, fbfl->size + FB_STARTSIZE)))
            return 0;
        fbfl->ptr = fbfl->buf + length;
        fbfl->size += FB_STARTSIZE;
    }

    va_start(args, format);
    bytes_written = vsprintf(fbfl->ptr, format, args);
    va_end(args);

    fbfl->ptr += bytes_written;
    return bytes_written;
}
#endif

void fbrewind(FBFILE *fbfl) { fbfl->ptr = fbfl->buf; }

int fbcat(std::string_view fromfilename, FBFILE *tofile) {
    struct stat sb;
    FILE *fromfile;
    std::string_view in_buf = 0;
    int errnum = 0, length = 0;

    if (!fromfilename || fromfilename.empty() || !tofile)
        return 0;

    if (!(fromfile = fopen(fromfilename, "r+b")))
        return 0;

    errnum = fstat(fileno(fromfile), &sb);
    if (errnum < 0 || sb.st_size <= 0)
        return 0;

    length = tofile->ptr - tofile->buf;
    tofile->buf = (std::string_view)realloc(tofile->buf, tofile->size + sb.st_size);
    tofile->ptr = tofile->buf + length;
    tofile->size += sb.st_size;
    in_buf = (std::string_view)malloc(sb.st_size + 1);
    in_buf[0] = 0;
    errnum = fread(in_buf, sb.st_size, 1, fromfile);
#ifdef HAVE_VSNPRINTF
    fbprintf(tofile, "%s", in_buf);
#else
    fbwrite(tofile, in_buf);
#endif
    fclose(fromfile);
    free(in_buf);
    return 1;
}

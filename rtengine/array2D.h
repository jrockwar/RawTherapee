/*
 *  This file is part of RawTherapee.
 *
 *  Copyright (c) 2011 Jan Rinze Peterzon (janrinze@gmail.com)
 *
 *  RawTherapee is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RawTherapee is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RawTherapee.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 *  Declaration of flexible 2D arrays
 *
 *  Usage:
 *
 *      array2D<type> name (X-size,Y-size);
 *      array2D<type> name (X-size,Y-size,type ** data);
 *
 *      creates an array which is valid within the normal C/C++ scope "{ ... }"
 *
 *      access to elements is as simple as:
 *
 *          array2D<float> my_array (10,10); // creates 10x10 array of floats
 *          value =  my_array[3][5];
 *          my_array[4][6]=value;
 *
 *      or copy an existing 2D array
 *
 *          float ** mydata;
 *          array2D<float> my_array (10,10,mydata);
 *
 *
 *      Useful extra pointers
 *
 *          <type> ** my_array      gives access to the pointer for access with [][]
 *          <type> *  my_array      gives access to the flat stored data.
 *
 *      Advanced usage:
 *          array2D<float> my_array             ; // empty container.
 *          my_array(10,10)                     ; // resize to 10x10 array
 *          my_array(10,10,ARRAY2D_CLEAR_DATA)  ; // resize to 10x10 and clear data
 *
 */
#pragma once

#include <cassert>
#include <cstring>
#include "noncopyable.h"

// flags for use
constexpr unsigned int ARRAY2D_CLEAR_DATA = 1;
constexpr unsigned int ARRAY2D_BYREFERENCE = 2;


template<typename T>
class array2D :
    public rtengine::NonCopyable
{

private:
    size_t width, height;
    T** rows;
    T* data;
    void ar_realloc(size_t w, size_t h, int offset = 0)
    {
        if (rows && (h > height || 4 * h < height)) {
            delete[] rows;
            rows = nullptr;
        }
        if (!rows) {
            rows = new T*[h];
        }

        if (data && ((h * w > width * height) || (h * w < (width * height / 4)))) {
            delete[] data;
            data = nullptr;
        }

        width = w;
        height = h;

        if (!data) {
            data = new T[height * width + offset];
        }

        for (size_t i = 0; i < height; i++) {
            rows[i] = data + offset + width * i;
        }
    }
public:

    // use as empty declaration, resize before use!
    // very useful as a member object
    array2D() :
        width(0), height(0), rows(nullptr), data(nullptr)
    { }

    // creator type1
    array2D(size_t w, size_t h, unsigned int flags = 0) :
        width(w), height(h)
    {
        data = new T[height * width];
        rows = new T*[height];

        for (size_t i = 0; i < height; ++i) {
            rows[i] = data + i * width;
        }

        if (flags & ARRAY2D_CLEAR_DATA) {
            memset(data, 0, width * height * sizeof(T));
        }
    }

    // creator type 2
    array2D(size_t w, size_t h, T ** source, unsigned int flags = 0) :
        width(w), height(h)
    {
        const bool owner = !(flags & ARRAY2D_BYREFERENCE);
        if (owner) {
            data = new T[height * width];
        } else {
            data = nullptr;
        }

        rows = new T*[height];

        for (size_t i = 0; i < height; ++i) {
            if (owner) {
                rows[i] = data + i * width;
                for (size_t j = 0; j < width; ++j) {
                    rows[i][j] = source[i][j];
                }
            } else {
                rows[i] = source[i];
            }
        }
    }

    // destructor
    ~array2D()
    {
        delete[] data;
        delete[] rows;
    }

    void fill(const T val, bool multiThread = false)
    {
#ifdef _OPENMP
        #pragma omp parallel for if(multiThread)
#endif
        for (size_t i = 0; i < width * height; ++i) {
            data[i] = val;
        }
    }

    void free()
    {
        delete[] data;
        data = nullptr;

        delete [] rows;
        rows = nullptr;
    }

    // use with indices
    T * operator[](size_t index)
    {
        assert((index >= 0) && (index < height));
        return rows[index];
    }

    const T * operator[](size_t index) const
    {
        assert((index >= 0) && (index < height));
        return rows[index];
    }

    // use as pointer to T**
    operator T**()
    {
        return rows;
    }

    // use as pointer to T**
    operator const T* const *() const
    {
        return rows;
    }

    // use as pointer to data
    operator T*()
    {
        // only if owner this will return a valid pointer
        return data;
    }

    operator const T*() const
    {
        // only if owner this will return a valid pointer
        return data;
    }


    // useful within init of parent object
    // or use as resize of 2D array
    void operator()(size_t w, size_t h, unsigned int flags = 0, int offset = 0)
    {
        ar_realloc(w, h, offset);

        if (flags & ARRAY2D_CLEAR_DATA) {
            memset(data + offset, 0, width * height * sizeof(T));
        }
    }

    size_t getWidth() const
    {
        return width;
    }
    size_t getHeight() const
    {
        return height;
    }

    operator bool()
    {
        return (width > 0 && height > 0);
    }

};
template<typename T, const size_t num>
class multi_array2D : public rtengine::NonCopyable
{
private:
    array2D<T> list[num];

public:
    multi_array2D(size_t width, size_t height, int flags = 0, int offset = 0)
    {
        for (size_t i = 0; i < num; ++i) {
            list[i](width, height, flags, (i + 1) * offset);
        }
    }

    array2D<T> & operator[](int index)
    {
        assert(static_cast<size_t>(index) < num);
        return list[index];
    }
};

/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file glamor_vbo.c
 *
 * Helpers for managing streamed vertex bufffers used in glamor.
 */

#include "glamor_priv.h"

/** Default size of the VBO, in bytes.
 *
 * If a single request is larger than this size, we'll resize the VBO
 * and return an appropriate mapping, but we'll resize back down after
 * that to avoid hogging that memory forever.  We don't anticipate
 * normal usage actually requiring larger VBO sizes.
 */
#define GLAMOR_VBO_SIZE (512 * 1024)

/**
 * Returns a pointer to @size bytes of VBO storage, which should be
 * accessed by the GL using vbo_offset within the VBO.
 */
void *
glamor_get_vbo_space(ScreenPtr screen, unsigned size, char **vbo_offset)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    void *data;

    glamor_get_context(glamor_priv);

    glBindBuffer(GL_ARRAY_BUFFER, glamor_priv->vbo);

    if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
        if (glamor_priv->vbo_size < glamor_priv->vbo_offset + size) {
            glamor_priv->vbo_size = MAX(GLAMOR_VBO_SIZE, size);
            glamor_priv->vbo_offset = 0;
            glBufferData(GL_ARRAY_BUFFER,
                         glamor_priv->vbo_size, NULL, GL_STREAM_DRAW);
        }

        data = glMapBufferRange(GL_ARRAY_BUFFER,
                                glamor_priv->vbo_offset,
                                size,
                                GL_MAP_WRITE_BIT |
                                GL_MAP_UNSYNCHRONIZED_BIT |
                                GL_MAP_INVALIDATE_RANGE_BIT);
        assert(data != NULL);
        *vbo_offset = (char *)(uintptr_t)glamor_priv->vbo_offset;
        glamor_priv->vbo_offset += size;
    } else {
        /* Return a pointer to the statically allocated non-VBO
         * memory. We'll upload it through glBufferData() later.
         */
        if (glamor_priv->vbo_size < size) {
            glamor_priv->vbo_size = MAX(GLAMOR_VBO_SIZE, size);
            free(glamor_priv->vb);
            glamor_priv->vb = XNFalloc(size);
        }
        *vbo_offset = NULL;
        /* We point to the start of glamor_priv->vb every time, and
         * the vbo_offset determines the size of the glBufferData().
         */
        glamor_priv->vbo_offset = size;
        data = glamor_priv->vb;
    }

    glamor_put_context(glamor_priv);

    return data;
}

void
glamor_put_vbo_space(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_get_context(glamor_priv);

    if (glamor_priv->gl_flavor == GLAMOR_GL_DESKTOP) {
        glUnmapBuffer(GL_ARRAY_BUFFER);
    } else {
        glBufferData(GL_ARRAY_BUFFER, glamor_priv->vbo_offset,
                     glamor_priv->vb, GL_DYNAMIC_DRAW);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glamor_put_context(glamor_priv);
}

void
glamor_init_vbo(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_get_context(glamor_priv);

    glGenBuffers(1, &glamor_priv->vbo);

    glamor_put_context(glamor_priv);
}

void
glamor_fini_vbo(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_get_context(glamor_priv);

    glDeleteBuffers(1, &glamor_priv->vbo);
    if (glamor_priv->gl_flavor != GLAMOR_GL_DESKTOP)
        free(glamor_priv->vb);

    glamor_put_context(glamor_priv);
}

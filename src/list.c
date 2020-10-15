/******************************************************************************
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2020 Joseph Kroesche
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *****************************************************************************/

#include <stdio.h>

#include "list.h"

// add item to the list
void list_add(struct list_node *p_head, struct list_node *p_node)
{
    // NOTE: no check for NULL
    p_node->p_next = p_head->p_next;
    p_head->p_next = p_node;
}

// remove item from the list
void list_remove(struct list_node *p_head, struct list_node *p_node)
{
    struct list_node *p_prev = p_head;
    struct list_node *p_next = p_prev->p_next;
    while (p_next != NULL)
    {
        if (p_next == p_node)
        {
            p_prev->p_next = p_node->p_next;
            p_node->p_next = NULL;
            break;
        }
        p_prev = p_next;
        p_next = p_prev->p_next;
    }
}

// iterate through a list
struct list_node *list_iter(struct list_node *p_node)
{
    if (p_node == NULL)
    {
        return NULL;
    }
    else
    {
        return p_node->p_next;
    }
}

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

#ifndef __LIST_H__
#define __LIST_H__

/**
 * @addtogroup list List
 *
 * A helper module for maintaining things in single-linked list.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * List node structure.
 */
struct list_node
{
    struct list_node *p_next;   ///< Pointer to next item in the list.
};

/**
 * Add an item to the list.
 *
 * @param p_head head of the list for adding an item
 * @param p_node the new item to add to the list
 *
 * The new item will be inserted at the head of the list.
 */
extern void list_add(struct list_node *p_head, struct list_node *p_node);

/**
 * Remove an item from the list.
 *
 * @param p_head head of the list for removing the item
 * @param p_node the item to remove from the list
 *
 * Walks the list and if _p_node_ is found, it will be removed from the list.
 * If the item is not found then there is no action.
 */
extern void list_remove(struct list_node *p_head, struct list_node *p_node);

/**
 * Walk the list returning one item for each call.
 *
 * @param p_node the list head or current item in the list
 *
 * The first time called, the parameter should be the list head. The function
 * will return the next item in the list. With each successive call, the
 * parameter should be item that was returned from the previous call. When the
 * end of the list is reached, NULL will be returned.
 *
 * @return the next item in the list or NULL.
 */
extern struct list_node *list_iter(struct list_node *p_node);

#ifdef __cplusplus
}
#endif

#endif

/** @} */

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

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>

#include "catch.hpp"
#include "list.h"

struct list_node listhead = { NULL };

TEST_CASE("list operations")
{
    listhead.p_next = NULL;
    struct list_node *pitem;
    struct list_node item1;
    struct list_node item2;
    struct list_node item3;

    SECTION("verify empty")
    {
        pitem = list_iter(&listhead);
        CHECK_FALSE(pitem);
    }

    SECTION("add item and remove")
    {
        // add item and verify it is there
        list_add(&listhead, &item1);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item1);

        // verify iter reaches end of list
        pitem = list_iter(pitem);
        CHECK_FALSE(pitem);

        // remove the item and verify list is empty
        list_remove(&listhead, &item1);
        pitem = list_iter(&listhead);
        CHECK_FALSE(pitem);
    }

    SECTION("add 2 items and remove in forward order")
    {
        // add first item and verify
        list_add(&listhead, &item1);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item1);

        pitem = list_iter(pitem);   // verify end of list
        CHECK_FALSE(pitem);

        // add second item and verify
        // check for both items in list
        list_add(&listhead, &item2);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item2);
        pitem = list_iter(pitem);
        CHECK(pitem == &item1);
        pitem = list_iter(pitem);
        CHECK_FALSE(pitem);

        // remove first item and verify
        list_remove(&listhead, &item1);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item2);
        pitem = list_iter(pitem);
        CHECK_FALSE(pitem);

        // remove second item and verify
        list_remove(&listhead, &item2);
        pitem = list_iter(&listhead);
        CHECK_FALSE(pitem);
    }

    SECTION("add 2 items and remove in reverse order")
    {
        // add first item and verify
        list_add(&listhead, &item1);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item1);

        pitem = list_iter(pitem);   // verify end of list
        CHECK_FALSE(pitem);

        // add second item and verify
        // check for both items in list
        list_add(&listhead, &item2);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item2);
        pitem = list_iter(pitem);
        CHECK(pitem == &item1);
        pitem = list_iter(pitem);
        CHECK_FALSE(pitem);

        // remove second item and verify
        list_remove(&listhead, &item2);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item1);
        pitem = list_iter(pitem);
        CHECK_FALSE(pitem);

        // remove first item and verify
        list_remove(&listhead, &item1);
        pitem = list_iter(&listhead);
        CHECK_FALSE(pitem);
    }

    SECTION("add 2 items and remove a non-existent item")
    {
        // add first item and verify
        list_add(&listhead, &item1);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item1);

        pitem = list_iter(pitem);   // verify end of list
        CHECK_FALSE(pitem);

        // add second item and verify
        // check for both items in list
        list_add(&listhead, &item2);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item2);
        pitem = list_iter(pitem);
        CHECK(pitem == &item1);
        pitem = list_iter(pitem);
        CHECK_FALSE(pitem);

        // remove non-existent third item,
        // verify list is unchanged
        list_remove(&listhead, &item3);
        pitem = list_iter(&listhead);
        CHECK(pitem == &item2);
        pitem = list_iter(pitem);
        CHECK(pitem == &item1);
        pitem = list_iter(pitem);
        CHECK_FALSE(pitem);
    }

    SECTION("remove item from empty list")
    {
        pitem = list_iter(&listhead);
        CHECK_FALSE(pitem);

        list_remove(&listhead, &item3);
        pitem = list_iter(&listhead);
        CHECK_FALSE(pitem);
    }
}

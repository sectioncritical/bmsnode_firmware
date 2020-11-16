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

#ifndef __KISSM_H__
#define __KISSM_H__

/** @addtogroup kissm KeepItSimple State Machine
 *
 * Example implementation:
 *
 * ~~~~~~~~{.c}
 * // declare app states
 * KISSM_DECLSTATE(app_start); // first app state
 * KISSM_DECLSTATE(appstate1);
 * KISSM_DECLSTATE(appstate2);
 *
 * KISSM_DEFSTATE(app_start)
 * {
 *     struct kissm_state *p_ns = NULL;
 *     switch (p_event->type)
 *     {
 *         case KISSM_EVT_ENTRY:
 *             // do entry actions
 *             break;
 *
 *         case KISSM_EVT_EXIT:
 *             // do exit actions
 *             break;
 *
 *         case APP_EVT:    // application defined event
 *             // conditionals based on event type and data
 *             p_ns = KISSM_STATEREF(appstate1);    // transition to state1
 *             break;
 *
 *         default:     // processing when there is no event
 *             // run background process, if any
 *             break;
 *     }
 *     return p_ns;
 * }
 *
 * KISSM_DEFSTATE(appstate1)
 * {
 *     // state handler body (see above for template)
 *     return p_ns;
 * }
 *
 * KISSM_DEFSTATE(appstate2)
 * {
 *     // state handler body (see above for template)
 *     return p_ns;
 * }
 *
 * ...
 *
 * // application main loop
 * int main(void)
 * {
 *     // app init stuff
 *
 *     // init the state machine with app first state
 *     kissm_init(KISSM_STATEREF(app_start);
 *
 *     // run loop
 *     for (;;)
 *     {
 *         // some app-specific method of collecting or generating events
 *         kissm_event * pevt = app_event_generator();
 *
 *         // KISSM_EVT_NONE may be a valid return from the event generator
 *
 *         // run the state machine
 *         kissm_run(pevt);
 *     }
 * }
 * ~~~~~~~~
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a reference to a named state struct. Useful for returning a new state
 * from a state handler.
 */
#define KISSM_STATEREF(s) &s##_state

/**
 * Declare a state handler and state struct to hold the state details. Useful
 * for creating forward declarations in the application.
 */
#define KISSM_DECLSTATE(s) struct kissm_state * s##_handler(const struct kissm_event *);  \
                           struct kissm_state s##_state={s##_handler,NULL}

/**
 * A convenience macro to define a state handler function. Use at the top of
 * the state handler implementation.
 */
#define KISSM_DEFSTATE(s) struct kissm_state *s##_handler(const struct kissm_event *p_event)

/**
 * Predefined event types. Application should extend with app-specific enum
 * starting with \ref KISSM_EVT_APP.
 *
 * ~~~~~~~~{.c}
 * enum myapp_event_type
 * {
 *     EVT_APPSPECIFIC1 = KISSM_EVT_APP,
 *     EVT_AOOSOECIFIC2,
 *     ...
 * };
 * ~~~~~~~~
 */
enum kissm_event_type
{
    KISSM_EVT_NONE = 0,   ///< No event occurred
    KISSM_EVT_ANY,        ///< (not used at this time)
    KISSM_EVT_EXIT,       ///< State is exiting
    KISSM_EVT_ENTRY,      ///< State is entering
    KISSM_EVT_APP         ///< Starting event type for app-specific events
};

/**
 * Union to allow event data to be pointer or byte data.
 */
union kissm_ptrbytes
{
    void *p;                    ///< data as a pointer
    uint8_t b[sizeof(void*)];   ///< data as bytes
};

/**
 * State machine event.
 */
struct kissm_event
{
    int type;                   ///< Type of event, \ref kissm_event_type or app-specific
    union kissm_ptrbytes data;  ///< Data associated with event, if any
};

/**
 * Defines a state "object" which contains all the information about a state.
 * Use convenience macros instead of referring to this directly.
 *
 * @sa KISSM_DECLSTATE, KISSM_STATEREF
 */
struct kissm_state
{
    struct kissm_state * (*handler)(const struct kissm_event *); ///< handler function
    void *pdata;    ///< State instance data, if any (not used at this time)
};

/**
 * Initialize the state machine with the application first state.
 *
 * @param init_state reference to the first application state
 *
 * This function must be called before using kissm_run(). It can also be
 * called at any time to reset the state machine.
 *
 * Example:
 *
 * ~~~~~~~~{.c}
 * DECLSTATE(app_start);
 * DECLSTATE(other_app_state);
 *
 * ...
 *
 * DEFSTATE(app_start)
 * {
 *     // state handler body
 * }
 *
 * ...
 *
 * // before main loop
 * // init the state machine
 * kissm_init(KISSM_STATEREF(app_start));
 *
 * ...
 * ~~~~~~~~
 */
extern void kissm_init(struct kissm_state *init_state);

/**
 * Run the state machine.
 *
 * @param p_event event to be passed to the state handler. May or may not cause
 *        a state transistion.
 *
 * This function should be called in order for the state machine handlers to
 * perform processing. Each time this is called, the present state handler
 * function will be called and passed the event. The state handler performs
 * whatever processing is needed, including possible causing a state
 * transition. State transitions are processed by this function when needed.
 * Upon a state transition, the present state handler will be called again
 * with the \ref KISSM_EVT_EXIT event which should be used to perform any exit
 * actions. And then the next state handler will be called with the event
 * \ref KISSM_EVT_ENTRY which can be used for performing entry actions.
 *
 * The frequency of calling this function depends on the application state
 * design. If states are designed to only perform any processing only when
 * there is an event, then this only needs to be called when there is a new
 * event. However, often a state will also require some ongoing processing to
 * run some process while in a state. In this case this function can be called
 * repeatedly with \ref KISSM_EVT_NONE which the state handler can use to
 * perform normal state processing.
 *
 * The application should implement a state machine ("main") loop that
 * collects or generates events, and pass those events to this function.
 */
extern void kissm_run(const struct kissm_event *p_event);

#ifdef __cplusplus
}
#endif

#endif

/** @} */

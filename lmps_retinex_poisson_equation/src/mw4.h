/**
 * @file lib/src/mw4.h
 *
 * @author Nicolas Limare <nicolas.limare@cmla.ens-cachan.fr>
 *
 * megawave 4 library api
 *
 * @todo formatted messages
 * @todo debug level instead of debug_flag
 * @todo option to remove all the debug code at compile time
 */

#ifndef _MW4_H_
#define _MW4_H_

int mw4_debug_flag;

/** print a message and abort the execution */
#define MW4_FATAL(MSG)					\
    do {						\
        fprintf(stderr, "fatal error: "MSG "\n");	\
	abort();					\
    } while (0);
/** print a formatted message and abort the execution */
#define MW4_FATALF(MSG, ...)					\
    do {							\
        fprintf(stderr, "fatal error: "MSG "\n", __VA_ARGS__);	\
	abort();						\
    } while (0);
/** print a warning message */
#define MW4_WARN(MSG)		      	\
    do {				\
        fprintf(stderr, MSG "\n");	\
    } while (0);
/** print a formatted warning message */
#define MW4_WARNF(MSG, ...)		      	\
    do {					\
        fprintf(stderr, MSG "\n", __VA_ARGS__);	\
    } while (0);
/** print an info message */
#define MW4_INFO(MSG)		      	\
    do {				\
        fprintf(stderr, MSG "\n");	\
    } while (0);
/** print a debug message */
#define MW4_DEBUG(MSG)							\
    do {								\
	if (mw4_debug_flag)						\
	    fprintf(stderr, __FILE__ ":%03i " MSG "\n", __LINE__);	\
    } while (0);
/** print a formatted debug message */
#define MW4_DEBUGF(MSG, ...)						\
    do {								\
	if (mw4_debug_flag)						\
	    fprintf(stderr, __FILE__ ":%03i " MSG "\n",			\
		    __LINE__, __VA_ARGS__);				\
    } while (0);

/** pre-defined message */
#define MW4_MSG_ALLOC_ERR "allocation error, not enough memory?"
/** pre-defined message */
#define MW4_MSG_BAD_PARAM "a function parameter has a bad value"
/** pre-defined message */
#define MW4_MSG_NULL_PTR "a pointer is NULL and should not be so"
/** pre-defined message */
#define MW4_MSG_FILE_READ_ERR "an error occured while reading a file"
/** pre-defined message */
#define MW4_MSG_PRECISION_LOSS "precision may be lost"

#endif /* !_MW4_H_ */

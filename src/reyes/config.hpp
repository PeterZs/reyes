#ifndef REYES_CONFIG_HPP_INCLUDED
#define REYES_CONFIG_HPP_INCLUDED

#if defined(BUILD_VARIANT_DEBUG)
#define REYES_ASSERT_ENABLED

#elif defined(BUILD_VARIANT_RELEASE)
#define REYES_ASSERT_ENABLED

#elif defined(BUILD_VARIANT_SHIPPING)

#endif

#endif
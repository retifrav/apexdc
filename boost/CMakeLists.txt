cmake_minimum_required(VERSION 3.22)

project("boost")

add_library(${PROJECT_NAME})

#file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/libs/*)
set(sources
    libs/chrono/src/chrono.cpp
    libs/chrono/src/process_cpu_clocks.cpp
    libs/chrono/src/thread_clock.cpp
    libs/filesystem/src/codecvt_error_category.cpp
    libs/filesystem/src/operations.cpp
    libs/filesystem/src/path.cpp
    libs/filesystem/src/path_traits.cpp
    libs/filesystem/src/portability.cpp
    libs/filesystem/src/unique_path.cpp
    libs/filesystem/src/utf8_codecvt_facet.cpp
    libs/filesystem/src/windows_file_codecvt.cpp
    libs/locale/src/encoding/codepage.cpp
    libs/locale/src/shared/date_time.cpp
    libs/locale/src/shared/format.cpp
    libs/locale/src/shared/formatting.cpp
    libs/locale/src/shared/generator.cpp
    libs/locale/src/shared/ids.cpp
    libs/locale/src/shared/localization_backend.cpp
    libs/locale/src/shared/message.cpp
    libs/locale/src/shared/mo_lambda.cpp
    libs/locale/src/util/codecvt_converter.cpp
    libs/locale/src/util/default_locale.cpp
    libs/locale/src/util/gregorian.cpp
    libs/locale/src/util/info.cpp
    libs/locale/src/util/locale_data.cpp
    libs/locale/src/win32/collate.cpp
    libs/locale/src/win32/converter.cpp
    libs/locale/src/win32/lcid.cpp
    libs/locale/src/win32/numeric.cpp
    libs/locale/src/win32/win_backend.cpp
    libs/log/src/attribute_name.cpp
    libs/log/src/attribute_set.cpp
    libs/log/src/attribute_value_set.cpp
    libs/log/src/code_conversion.cpp
    libs/log/src/core.cpp
    libs/log/src/date_time_format_parser.cpp
    libs/log/src/default_attribute_names.cpp
    libs/log/src/default_sink.cpp
    libs/log/src/dump.cpp
    libs/log/src/event.cpp
    libs/log/src/exceptions.cpp
    libs/log/src/format_parser.cpp
    libs/log/src/global_logger_storage.cpp
    libs/log/src/named_scope.cpp
    libs/log/src/named_scope_format_parser.cpp
    libs/log/src/once_block.cpp
    libs/log/src/permissions.cpp
    libs/log/src/process_id.cpp
    libs/log/src/process_name.cpp
    libs/log/src/record_ostream.cpp
    libs/log/src/setup/default_filter_factory.cpp
    libs/log/src/setup/default_formatter_factory.cpp
    libs/log/src/setup/filter_parser.cpp
    libs/log/src/setup/formatter_parser.cpp
    libs/log/src/setup/init_from_settings.cpp
    libs/log/src/setup/init_from_stream.cpp
    libs/log/src/setup/matches_relation_factory.cpp
    libs/log/src/setup/parser_utils.cpp
    libs/log/src/severity_level.cpp
    libs/log/src/spirit_encoding.cpp
    libs/log/src/syslog_backend.cpp
    libs/log/src/text_file_backend.cpp
    libs/log/src/text_multifile_backend.cpp
    libs/log/src/text_ostream_backend.cpp
    libs/log/src/thread_id.cpp
    libs/log/src/thread_specific.cpp
    libs/log/src/threadsafe_queue.cpp
    libs/log/src/timer.cpp
    libs/log/src/timestamp.cpp
    libs/log/src/trivial.cpp
    libs/log/src/unhandled_exception_count.cpp
    libs/log/src/windows/debug_output_backend.cpp
    libs/log/src/windows/event_log_backend.cpp
    libs/log/src/windows/ipc_reliable_message_queue.cpp
    libs/log/src/windows/ipc_sync_wrappers.cpp
    libs/log/src/windows/light_rw_mutex.cpp
    libs/log/src/windows/mapped_shared_memory.cpp
    libs/log/src/windows/object_name.cpp
    libs/random/src/random_device.cpp
    libs/regex/src/c_regex_traits.cpp
    libs/regex/src/cpp_regex_traits.cpp
    libs/regex/src/cregex.cpp
    libs/regex/src/fileiter.cpp
    libs/regex/src/icu.cpp
    libs/regex/src/instances.cpp
    libs/regex/src/posix_api.cpp
    libs/regex/src/regex.cpp
    libs/regex/src/regex_debug.cpp
    libs/regex/src/regex_raw_buffer.cpp
    libs/regex/src/regex_traits_defaults.cpp
    libs/regex/src/static_mutex.cpp
    libs/regex/src/usinstances.cpp
    libs/regex/src/w32_regex_traits.cpp
    libs/regex/src/wc_regex_traits.cpp
    libs/regex/src/wide_posix_api.cpp
    libs/regex/src/winstances.cpp
    libs/serialization/src/archive_exception.cpp
    libs/serialization/src/basic_archive.cpp
    libs/serialization/src/basic_iarchive.cpp
    libs/serialization/src/basic_iserializer.cpp
    libs/serialization/src/basic_oarchive.cpp
    libs/serialization/src/basic_oserializer.cpp
    libs/serialization/src/basic_pointer_iserializer.cpp
    libs/serialization/src/basic_pointer_oserializer.cpp
    libs/serialization/src/basic_serializer_map.cpp
    libs/serialization/src/basic_text_iprimitive.cpp
    libs/serialization/src/basic_text_oprimitive.cpp
    libs/serialization/src/basic_text_wiprimitive.cpp
    libs/serialization/src/basic_text_woprimitive.cpp
    libs/serialization/src/basic_xml_archive.cpp
    libs/serialization/src/binary_iarchive.cpp
    libs/serialization/src/binary_oarchive.cpp
    libs/serialization/src/binary_wiarchive.cpp
    libs/serialization/src/binary_woarchive.cpp
    libs/serialization/src/codecvt_null.cpp
    libs/serialization/src/extended_type_info.cpp
    libs/serialization/src/extended_type_info_no_rtti.cpp
    libs/serialization/src/extended_type_info_typeid.cpp
    libs/serialization/src/polymorphic_iarchive.cpp
    libs/serialization/src/polymorphic_oarchive.cpp
    libs/serialization/src/stl_port.cpp
    libs/serialization/src/text_iarchive.cpp
    libs/serialization/src/text_oarchive.cpp
    libs/serialization/src/text_wiarchive.cpp
    libs/serialization/src/text_woarchive.cpp
    libs/serialization/src/utf8_codecvt_facet.cpp
    libs/serialization/src/void_cast.cpp
    libs/serialization/src/xml_archive_exception.cpp
    libs/serialization/src/xml_grammar.cpp
    libs/serialization/src/xml_iarchive.cpp
    libs/serialization/src/xml_oarchive.cpp
    libs/serialization/src/xml_wgrammar.cpp
    libs/serialization/src/xml_wiarchive.cpp
    libs/serialization/src/xml_woarchive.cpp
    libs/system/src/error_code.cpp
    libs/thread/src/future.cpp
    libs/thread/src/tss_null.cpp
    libs/thread/src/win32/thread.cpp
    libs/thread/src/win32/thread_primitives.cpp
    libs/thread/src/win32/tss_dll.cpp
    libs/thread/src/win32/tss_pe.cpp
    libs/timer/src/auto_timers_construction.cpp
    libs/timer/src/cpu_timer.cpp
)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${sources}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/chrono/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/filesystem/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/locale/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/log/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/random/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/regex/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/serialization/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/system/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/thread/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/timer/src
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # where external projects will look for the library's public headers
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        "BOOST_ALL_NO_LIB"
        "BOOST_USE_WINDOWS_H"
        "BOOST_LOG_WITHOUT_EVENT_LOG"
        "BOOST_LOG_WITHOUT_SETTINGS_PARSERS"
        "BOOST_LOG_WITHOUT_SYSLOG"
        "BOOST_SPIRIT_USE_PHOENIX_V3=1"
        "BOOST_LOCALE_NO_POSIX_BACKEND=1"
        "BOOST_LOCALE_NO_STD_BACKEND=1"
        "SECURITY_WIN32"
        "NOMINMAX"
        "_SECURE_SCL=0"
        "_ITERATOR_DEBUG_LEVEL=0"
        "_HAS_ITERATOR_DEBUGGING=0"
        "_WIN32_WINNT=0x600"
        "WINVER=0x600"
)

# target_compile_options(${PROJECT_NAME}
#     PRIVATE
#         $<$<CONFIG:DebugXP>:
#             /Od;
#             /RTC1
#         >
#         $<$<CONFIG:Debug>:
#             /Od;
#             /RTC1
#         >
#         $<$<CONFIG:ReleaseXP>:
#             /Ox;
#             /Ob1;
#             /Oi;
#             /GF;
#             /GT;
#             /Gy;
#             /Os;
#             /fp:fast;
#             /GS-
#         >
#         $<$<CONFIG:Release>:
#             /Ox;
#             /Ob1;
#             /Oi;
#             /GF;
#             /GT;
#             /Gy;
#             /Os;
#             /fp:fast;
#             /GS-
#         >
#         /MP;
#         /Oy-;
#         /Zi;
#         /GR;
#         /wd4221;
#         ${DEFAULT_CXX_EXCEPTION_HANDLING}
# )

# target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#         zlib
# )

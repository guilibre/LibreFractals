add_executable(Backend
    main.cpp
    controllers/gallery.cpp
    routes/gallery.cpp
    store/gallery.cpp
    util/encoding.cpp
    util/http.cpp
    util/json.cpp
)

target_link_libraries(Backend PRIVATE ${DEPS} ZLIB::ZLIB)

#ifndef CROSSPATH_H
#define CROSSPATH_H
#include <string_view>
#include <string>


class Crosspath {
    std::string home;

public:
    Crosspath();

    std::string data() const;
    std::string config() const;
    std::string cache() const;
    std::string temp() const;
};

#endif // CROSSPATH_H

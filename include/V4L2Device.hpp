#ifndef V4L2_DEVICE_HPP
#define V4L2_DEVICE_HPP


#include <algorithm>
#include <string>
#include <cstring>
#include <vector>
#include <vector>
#include <map>

using dev_vec = std::vector<std::string>;
using dev_map = std::map<std::string, std::string>;


/*
 * references
 *   - https://git.linuxtv.org/v4l-utils.git/tree/utils/v4l2-ctl/v4l2-ctl-common.cpp
 */
class V4L2Device
{
private:
    dev_map file_card_map;

    bool _is_v4l_dev(const char *name);
    int _calc_node_val(const char *s);
    bool _sort_on_device_name(const std::string &s1, const std::string &s2);

protected:
    bool _configure_camera(const std::string &dev_file);

    void _init_dev_files(void);

    bool _find_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file);
    bool _find_and_remove_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file);
    bool _find_and_remove_dev_file_by_strs(const std::vector<std::string> &id_strs, std::string &dev_file, std::string &found_id_str);
};

#endif // V4L2_DEVICE_HPP

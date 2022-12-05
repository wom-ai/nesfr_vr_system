#ifndef VR_INPUT_HPP
#define VR_INPUT_HPP

struct head_control_input_t {
    uint32_t    seq_num;                // 0
    float       roll;                   // 4
    float       pitch;                  // 8
    float       yaw;                    //12
    uint8_t     move_state;             //16
    uint8_t     predefined_move_state;  //17
};

struct right_control_input_t {
    uint32_t    seq_num;                // 0
    float       joystick_x;             // 4
    float       joystick_y;             // 8
    float       index;                  //12
    float       grip;                   //16
    bool        button_a;               //20
    bool        button_b;               //21
    uint8_t     move_state;             //22
    uint8_t     predefined_move_state;  //23
};

struct left_control_input_t {
    uint32_t    seq_num;                // 0
    float       joystick_x;             // 4
    float       joystick_y;             // 8
    float       index;                  //12
    float       grip;                   //16
    bool        button_x;               //20
    bool        button_y;               //21
    uint8_t     move_state;             //22
    uint8_t     predefined_move_state;  //23
};

struct vr_input_t {
    uint32_t    seq_num;
    struct head_control_input_t;
    struct left_control_input_t;
    struct right_control_input_t;
};

#endif // VR_INPUT_HPP

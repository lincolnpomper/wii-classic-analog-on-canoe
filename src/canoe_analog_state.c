#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdint.h>

typedef union {
    uint32_t type;
    struct {
        uint32_t type, timestamp, which;
        uint8_t axis, padding1, padding2, padding3;
        int16_t value;
        uint16_t padding4;
    } caxis;
    uint8_t pad[56];
} SDL_Event_Compat;

typedef int (*poll_event_fn)(void *);
typedef uint8_t (*get_button_fn)(void *, int);
typedef void *(*get_joystick_fn)(void *);
typedef int32_t (*joystick_instance_id_fn)(void *);

enum {
    SDL_CONTROLLERAXISMOTION = 0x650,
    DPAD_UP = 11,
    DPAD_DOWN = 12,
    DPAD_LEFT = 13,
    DPAD_RIGHT = 14,
    ENTER_THRESHOLD = 18000,
    EXIT_THRESHOLD = 12000
};

static poll_event_fn real_poll;
static get_button_fn real_get_button;
static get_joystick_fn real_get_joystick;
static joystick_instance_id_fn real_joystick_instance_id;

/* SDL identifies controller events by joystick instance ID.  Keeping a state
 * per ID prevents one Classic Controller from affecting another player. */
struct controller_state {
    int32_t instance_id;
    uint8_t analog_down[4]; /* SDL d-pad order: up, down, left, right. */
};
static struct controller_state controller_states[4];
static int states_initialized;

static struct controller_state *state_for(int32_t instance_id, int create) {
    int i;
    if (!states_initialized) {
        for (i = 0; i < 4; ++i)
            controller_states[i].instance_id = -1;
        states_initialized = 1;
    }
    for (i = 0; i < 4; ++i)
        if (controller_states[i].instance_id == instance_id)
            return &controller_states[i];
    if (!create)
        return 0;
    for (i = 0; i < 4; ++i) {
        if (controller_states[i].instance_id == -1) {
            controller_states[i].instance_id = instance_id;
            return &controller_states[i];
        }
    }
    return 0;
}

static uint8_t positive(int16_t value, uint8_t was_down) {
    return was_down ? value > EXIT_THRESHOLD : value > ENTER_THRESHOLD;
}

static uint8_t negative(int16_t value, uint8_t was_down) {
    return was_down ? value < -EXIT_THRESHOLD : value < -ENTER_THRESHOLD;
}

int SDL_PollEvent(void *opaque_event) {
    SDL_Event_Compat *event = opaque_event;
    struct controller_state *state;
    int result;

    if (!real_poll)
        real_poll = (poll_event_fn)dlsym(RTLD_NEXT, "SDL_PollEvent");
    if (!real_poll)
        return 0;
    result = real_poll(opaque_event);
    if (!result || !event || event->type != SDL_CONTROLLERAXISMOTION)
        return result;

    state = state_for((int32_t)event->caxis.which, 1);
    if (!state)
        return result;

    if (event->caxis.axis == 0) {
        state->analog_down[3] = positive(event->caxis.value, state->analog_down[3]); /* right */
        state->analog_down[2] = negative(event->caxis.value, state->analog_down[2]); /* left */
    } else if (event->caxis.axis == 1) {
        state->analog_down[1] = positive(event->caxis.value, state->analog_down[1]); /* down */
        state->analog_down[0] = negative(event->caxis.value, state->analog_down[0]); /* up */
    }
    return result;
}

/* Canoe polls SDL_GameControllerGetButton for movement.  The original D-pad
 * state is always retained; the left stick simply adds a digital press. */
uint8_t SDL_GameControllerGetButton(void *controller, int button) {
    uint8_t actual = 0;
    struct controller_state *state = 0;
    if (!real_get_button)
        real_get_button = (get_button_fn)dlsym(RTLD_NEXT, "SDL_GameControllerGetButton");
    if (!real_get_joystick)
        real_get_joystick = (get_joystick_fn)dlsym(RTLD_NEXT, "SDL_GameControllerGetJoystick");
    if (!real_joystick_instance_id)
        real_joystick_instance_id = (joystick_instance_id_fn)dlsym(RTLD_NEXT, "SDL_JoystickInstanceID");
    if (real_get_button)
        actual = real_get_button(controller, button);
    if (button >= DPAD_UP && button <= DPAD_RIGHT && real_get_joystick && real_joystick_instance_id)
        state = state_for(real_joystick_instance_id(real_get_joystick(controller)), 0);
    if (state)
        return actual || state->analog_down[button - DPAD_UP];
    return actual;
}

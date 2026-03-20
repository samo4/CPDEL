/**
 * @file screen_manager.h
 * Screen transition management for the DC Load GUI.
 */

#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        SCREEN_MAIN = 0,
        SCREEN_CHANNEL_DETAIL,
        SCREEN_GRAPH,
        SCREEN_SETTINGS,
        SCREEN_COUNT
    } screen_id_t;

    /**
     * Initialise all screens and display the main screen.
     */
    void screen_manager_init(void);

    /**
     * Navigate to the specified screen.
     * For SCREEN_CHANNEL_DETAIL and SCREEN_GRAPH, channel_idx selects CH1 (0) / CH2 (1).
     * Pass 0 for screens that don't need a channel index.
     */
    void screen_manager_goto(screen_id_t id, int channel_idx);

    /**
     * Refresh the currently displayed screen with new data.
     * Call regularly from the main loop.
     */
    void screen_manager_refresh(void);

    /**
     * Returns currently active screen id.
     */
    screen_id_t screen_manager_current(void);

#ifdef __cplusplus
}
#endif

#endif /* SCREEN_MANAGER_H */

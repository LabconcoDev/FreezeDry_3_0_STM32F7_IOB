#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

/* ======================================================== */
/*  SIMULATION SWITCH                                 */
/* Comment out this line to return to physical hardware!    */
/* ======================================================== */
#define SIMULATION 1
#ifdef SIMULATION
    /* 1 = Simulate TRIAD (Tilt Valve Present = true,  Vent Valve Present = true)
     * 0 = Simulate FD    (Tilt Valve Present = false, Vent Valve Present = true)
     */
    #define SIMULATE_AS_TRIAD   1
#endif

#endif /* BUILD_CONFIG_H */

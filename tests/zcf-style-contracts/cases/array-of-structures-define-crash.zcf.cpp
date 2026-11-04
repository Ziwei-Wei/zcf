PANEL_Ic PANEL_ic[PANEL_IC_NUMBER] = {
    {PIC(0), PIC(0), PIC(99), PIC(81), 0},    // Backbox
    {PIC(1), PIC(83), PIC(191), PIC(137), 0}, // AK47

#define PICALL1(a, b, c, d) {PIC(a), PIC(b), PIC(c), PIC(d), 1}

    PICALL1(1, 1, 75, 50),
};

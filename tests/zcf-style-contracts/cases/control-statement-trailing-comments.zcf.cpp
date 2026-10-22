void update(int value) {
  prepare(); // before the control statement

  if (value) {
    apply();
  } // after the control statement

  finish(); /* following statement */
}

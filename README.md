# papaver

Issues
1. gcc inline assembly registers conflict when using memory varialbes with cast type
  For inline assembly statement leaded by "__asm__", with some memory variables as input/output,
the auto assigned registers for cast memory variables may conflict with the registers used by assembly statement.

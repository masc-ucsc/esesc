
// mips-img-linux-gnu-gcc -EL -g -mabi=64 -mips64r6 -Os  -static ./startroi.c -o startroi
// mips-img-linux-gnu-strip startroi
int main() {
   // Privilieged instruction: asm volatile(".word 0x42000021");
  asm volatile(".word 0x28"); // spim instruction
  return 0;
}

#!/usr/bin/env ruby
in_vminst = false
instructions = Hash.new
STDIN.each do |line|
  if /^enum vminst {/ =~ line
    in_vminst = true;
  end
  next if !in_vminst
  if /^\s+i(.*)=([0-9]+)/ =~ line
    instructions[$2.to_i] = $1.clone
  elsif /^};$/ =~ line
    break
  end
end

instrlist = []
puts "static struct {
  char *inst;
  int argc;
  int jmpflg;
} disasmtbl[] = {"
0.upto(255) do |index|
  if instructions.has_key?(index)
    nargs = (index & 0xc0) >> 6
    jmpflg = (/(jmp|jf|jt|cont)/ === instructions[index]) ? 1 : 0
    instrlist << "  { \"#{instructions[index]}\", #{nargs}, #{jmpflg}}"
  else
    instrlist << sprintf("  { \"?? %02x\", 0 }", index)
  end
end
puts instrlist.join(",\n")
puts "};"

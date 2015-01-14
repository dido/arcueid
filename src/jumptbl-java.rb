#!/usr/bin/env ruby
# Generate Java jumptbl.
#
in_vminst = false
instructions = Hash.new
STDIN.each do |line|
  if /^enum vminst {/ =~ line
    in_vminst = true;
  end
  next if !in_vminst
  if /^\s+(i.*)=([0-9]+)/ =~ line
    instructions[$2.to_i] = ($1.clone[1..-1]).upcase
  elsif /^};$/ =~ line
    break
  end
end
puts "private static final Instruction[] jmptbl = {"
0.upto(255) do |opcode|
  if instructions.has_key?(opcode)
    puts "new #{instructions[opcode]}(),"
  else
    puts "NOINST,"
  end
end
puts "};"


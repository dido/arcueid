/* 
  Copyright (C) 2009 Rafael R. Sevilla

  This file is part of CArc

  CArc is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
*/
enum
{
  MAGIC_A = 0xa110c,	     /* Allocated block */
  MAGIC_F = 0xbadc0c0a,	     /* Free block */
  MAGIC_E = 0xdeadbabe,	     /* End of arena */
  MAGIC_I = 0xabba	     /* Block is immutable (hidden from gc) */
};


struct Bhdr {
  uint64_t magic;		/* magic number code of the block */
  uint64_t size;		/* size of the block */
  union {
    uint8_t data[1];		/* data in the block (if allocated) */
    struct {
      struct Bhdr *left;	/* left child of this node */
      struct Bhdr *right;	/* right child of this node */
    };
  } s;
};

#define B2D(bp) ((void *)(bp)->u.data)

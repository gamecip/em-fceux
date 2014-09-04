/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

static void ops00(void) { do {  /* BRK */
            _PC++;
            PUSH(_PC>>8);
            PUSH(_PC);
            PUSH(_P|U_FLAG|B_FLAG);
	    _P|=I_FLAG;
	    _PI|=I_FLAG;
            _PC=RdMem(0xFFFE);
            _PC|=RdMem(0xFFFF)<<8;
            break;

} while(0); } static void ops40(void) { do {  /* RTI */
            _P=POP();
	    /* _PI=_P; This is probably incorrect, so it's commented out. */
	    _PI = _P;
            _PC=POP();
            _PC|=POP()<<8;
            break;
            
} while(0); } static void ops60(void) { do {  /* RTS */
            _PC=POP();
            _PC|=POP()<<8;
            _PC++;
            break;

} while(0); } static void ops48(void) { do { /* PHA */
           PUSH(_A);
           break;
} while(0); } static void ops08(void) { do { /* PHP */
           PUSH(_P|U_FLAG|B_FLAG);
           break;
} while(0); } static void ops68(void) { do { /* PLA */
           _A=POP();
           X_ZN(_A);
           break;
} while(0); } static void ops28(void) { do { /* PLP */
           _P=POP();
           break;
} while(0); } static void ops4C(void) { do {
	  {
	   uint16 ptmp=_PC;
	   unsigned int npc;

	   npc=RdMem(ptmp);
	   ptmp++;
	   npc|=RdMem(ptmp)<<8;
	   _PC=npc;
	  }
	  break; /* JMP ABSOLUTE */
} while(0); } static void ops6C(void) { do {
	   {
	    uint32 tmp;
	    GetAB(tmp);
	    _PC=RdMem(tmp);
	    _PC|=RdMem( ((tmp+1)&0x00FF) | (tmp&0xFF00))<<8;
	   }
	   break;
} while(0); } static void ops20(void) { do { /* JSR */
	   {
	    uint8 npc;
	    npc=RdMem(_PC);
	    _PC++;
            PUSH(_PC>>8);
            PUSH(_PC);
            _PC=RdMem(_PC)<<8;
	    _PC|=npc;
	   }
           break;

} while(0); } static void opsAA(void) { do { /* TAX */
           _X=_A;
           X_ZN(_A);
           break;

} while(0); } static void ops8A(void) { do { /* TXA */
           _A=_X;
           X_ZN(_A);
           break;

} while(0); } static void opsA8(void) { do { /* TAY */
           _Y=_A;
           X_ZN(_A);
           break;
} while(0); } static void ops98(void) { do { /* TYA */
           _A=_Y;
           X_ZN(_A);
           break;

} while(0); } static void opsBA(void) { do { /* TSX */
           _X=_S;
           X_ZN(_X);
           break;
} while(0); } static void ops9A(void) { do { /* TXS */
           _S=_X;
           break;

} while(0); } static void opsCA(void) { do { /* DEX */
           _X--;
           X_ZN(_X);
           break;
} while(0); } static void ops88(void) { do { /* DEY */
           _Y--;
           X_ZN(_Y);
           break;

} while(0); } static void opsE8(void) { do { /* INX */
           _X++;
           X_ZN(_X);
           break;
} while(0); } static void opsC8(void) { do { /* INY */
           _Y++;
           X_ZN(_Y);
           break;

} while(0); } static void ops18(void) { do { /* CLC */
           _P&=~C_FLAG;
           break;
} while(0); } static void opsD8(void) { do { /* CLD */
           _P&=~D_FLAG;
           break;
} while(0); } static void ops58(void) { do { /* CLI */
           _P&=~I_FLAG;
           break;
} while(0); } static void opsB8(void) { do { /* CLV */
           _P&=~V_FLAG;
           break;

} while(0); } static void ops38(void) { do { /* SEC */
           _P|=C_FLAG;
           break;
} while(0); } static void opsF8(void) { do { /* SED */
           _P|=D_FLAG;
           break;
} while(0); } static void ops78(void) { do { /* SEI */
           _P|=I_FLAG;
           break;

} while(0); } static void opsEA(void) { do { /* NOP */
           break;

} while(0); } static void ops0A(void) { do { RMW_A(ASL);
} while(0); } static void ops06(void) { do { RMW_ZP(ASL);
} while(0); } static void ops16(void) { do { RMW_ZPX(ASL);
} while(0); } static void ops0E(void) { do { RMW_AB(ASL);
} while(0); } static void ops1E(void) { do { RMW_ABX(ASL);

} while(0); } static void opsC6(void) { do { RMW_ZP(DEC);
} while(0); } static void opsD6(void) { do { RMW_ZPX(DEC);
} while(0); } static void opsCE(void) { do { RMW_AB(DEC);
} while(0); } static void opsDE(void) { do { RMW_ABX(DEC);

} while(0); } static void opsE6(void) { do { RMW_ZP(INC);
} while(0); } static void opsF6(void) { do { RMW_ZPX(INC);
} while(0); } static void opsEE(void) { do { RMW_AB(INC);
} while(0); } static void opsFE(void) { do { RMW_ABX(INC);

} while(0); } static void ops4A(void) { do { RMW_A(LSR);
} while(0); } static void ops46(void) { do { RMW_ZP(LSR);
} while(0); } static void ops56(void) { do { RMW_ZPX(LSR);
} while(0); } static void ops4E(void) { do { RMW_AB(LSR);
} while(0); } static void ops5E(void) { do { RMW_ABX(LSR);

} while(0); } static void ops2A(void) { do { RMW_A(ROL);
} while(0); } static void ops26(void) { do { RMW_ZP(ROL);
} while(0); } static void ops36(void) { do { RMW_ZPX(ROL);
} while(0); } static void ops2E(void) { do { RMW_AB(ROL);
} while(0); } static void ops3E(void) { do { RMW_ABX(ROL);

} while(0); } static void ops6A(void) { do { RMW_A(ROR);
} while(0); } static void ops66(void) { do { RMW_ZP(ROR);
} while(0); } static void ops76(void) { do { RMW_ZPX(ROR);
} while(0); } static void ops6E(void) { do { RMW_AB(ROR);
} while(0); } static void ops7E(void) { do { RMW_ABX(ROR);

} while(0); } static void ops69(void) { do { LD_IM(ADC);
} while(0); } static void ops65(void) { do { LD_ZP(ADC);
} while(0); } static void ops75(void) { do { LD_ZPX(ADC);
} while(0); } static void ops6D(void) { do { LD_AB(ADC);
} while(0); } static void ops7D(void) { do { LD_ABX(ADC);
} while(0); } static void ops79(void) { do { LD_ABY(ADC);
} while(0); } static void ops61(void) { do { LD_IX(ADC);
} while(0); } static void ops71(void) { do { LD_IY(ADC);

} while(0); } static void ops29(void) { do { LD_IM(AND);
} while(0); } static void ops25(void) { do { LD_ZP(AND);
} while(0); } static void ops35(void) { do { LD_ZPX(AND);
} while(0); } static void ops2D(void) { do { LD_AB(AND);
} while(0); } static void ops3D(void) { do { LD_ABX(AND);
} while(0); } static void ops39(void) { do { LD_ABY(AND);
} while(0); } static void ops21(void) { do { LD_IX(AND);
} while(0); } static void ops31(void) { do { LD_IY(AND);

} while(0); } static void ops24(void) { do { LD_ZP(BIT);
} while(0); } static void ops2C(void) { do { LD_AB(BIT);

} while(0); } static void opsC9(void) { do { LD_IM(CMP);
} while(0); } static void opsC5(void) { do { LD_ZP(CMP);
} while(0); } static void opsD5(void) { do { LD_ZPX(CMP);
} while(0); } static void opsCD(void) { do { LD_AB(CMP);
} while(0); } static void opsDD(void) { do { LD_ABX(CMP);
} while(0); } static void opsD9(void) { do { LD_ABY(CMP);
} while(0); } static void opsC1(void) { do { LD_IX(CMP);
} while(0); } static void opsD1(void) { do { LD_IY(CMP);

} while(0); } static void opsE0(void) { do { LD_IM(CPX);
} while(0); } static void opsE4(void) { do { LD_ZP(CPX);
} while(0); } static void opsEC(void) { do { LD_AB(CPX);

} while(0); } static void opsC0(void) { do { LD_IM(CPY);
} while(0); } static void opsC4(void) { do { LD_ZP(CPY);
} while(0); } static void opsCC(void) { do { LD_AB(CPY);

} while(0); } static void ops49(void) { do { LD_IM(EOR);
} while(0); } static void ops45(void) { do { LD_ZP(EOR);
} while(0); } static void ops55(void) { do { LD_ZPX(EOR);
} while(0); } static void ops4D(void) { do { LD_AB(EOR);
} while(0); } static void ops5D(void) { do { LD_ABX(EOR);
} while(0); } static void ops59(void) { do { LD_ABY(EOR);
} while(0); } static void ops41(void) { do { LD_IX(EOR);
} while(0); } static void ops51(void) { do { LD_IY(EOR);

} while(0); } static void opsA9(void) { do { LD_IM(LDA);
} while(0); } static void opsA5(void) { do { LD_ZP(LDA);
} while(0); } static void opsB5(void) { do { LD_ZPX(LDA);
} while(0); } static void opsAD(void) { do { LD_AB(LDA);
} while(0); } static void opsBD(void) { do { LD_ABX(LDA);
} while(0); } static void opsB9(void) { do { LD_ABY(LDA);
} while(0); } static void opsA1(void) { do { LD_IX(LDA);
} while(0); } static void opsB1(void) { do { LD_IY(LDA);

} while(0); } static void opsA2(void) { do { LD_IM(LDX);
} while(0); } static void opsA6(void) { do { LD_ZP(LDX);
} while(0); } static void opsB6(void) { do { LD_ZPY(LDX);
} while(0); } static void opsAE(void) { do { LD_AB(LDX);
} while(0); } static void opsBE(void) { do { LD_ABY(LDX);

} while(0); } static void opsA0(void) { do { LD_IM(LDY);
} while(0); } static void opsA4(void) { do { LD_ZP(LDY);
} while(0); } static void opsB4(void) { do { LD_ZPX(LDY);
} while(0); } static void opsAC(void) { do { LD_AB(LDY);
} while(0); } static void opsBC(void) { do { LD_ABX(LDY);

} while(0); } static void ops09(void) { do { LD_IM(ORA);
} while(0); } static void ops05(void) { do { LD_ZP(ORA);
} while(0); } static void ops15(void) { do { LD_ZPX(ORA);
} while(0); } static void ops0D(void) { do { LD_AB(ORA);
} while(0); } static void ops1D(void) { do { LD_ABX(ORA);
} while(0); } static void ops19(void) { do { LD_ABY(ORA);
} while(0); } static void ops01(void) { do { LD_IX(ORA);
} while(0); } static void ops11(void) { do { LD_IY(ORA);

} while(0); } static void opsE9(void) { do { LD_IM(SBC);
} while(0); } static void opsEB(void) { do { opsE9(); /* (undocumented) */
} while(0); } static void opsE5(void) { do { LD_ZP(SBC);
} while(0); } static void opsF5(void) { do { LD_ZPX(SBC);
} while(0); } static void opsED(void) { do { LD_AB(SBC);
} while(0); } static void opsFD(void) { do { LD_ABX(SBC);
} while(0); } static void opsF9(void) { do { LD_ABY(SBC);
} while(0); } static void opsE1(void) { do { LD_IX(SBC);
} while(0); } static void opsF1(void) { do { LD_IY(SBC);

} while(0); } static void ops85(void) { do { ST_ZP(_A);
} while(0); } static void ops95(void) { do { ST_ZPX(_A);
} while(0); } static void ops8D(void) { do { ST_AB(_A);
} while(0); } static void ops9D(void) { do { ST_ABX(_A);
} while(0); } static void ops99(void) { do { ST_ABY(_A);
} while(0); } static void ops81(void) { do { ST_IX(_A);
} while(0); } static void ops91(void) { do { ST_IY(_A);

} while(0); } static void ops86(void) { do { ST_ZP(_X);
} while(0); } static void ops96(void) { do { ST_ZPY(_X);
} while(0); } static void ops8E(void) { do { ST_AB(_X);

} while(0); } static void ops84(void) { do { ST_ZP(_Y);
} while(0); } static void ops94(void) { do { ST_ZPX(_Y);
} while(0); } static void ops8C(void) { do { ST_AB(_Y);

/* BCC */
} while(0); } static void ops90(void) { do { JR(!(_P&C_FLAG)); break;

/* BCS */
} while(0); } static void opsB0(void) { do { JR(_P&C_FLAG); break;

/* BEQ */
} while(0); } static void opsF0(void) { do { JR(_P&Z_FLAG); break;

/* BNE */
} while(0); } static void opsD0(void) { do { JR(!(_P&Z_FLAG)); break;

/* BMI */
} while(0); } static void ops30(void) { do { JR(_P&N_FLAG); break;

/* BPL */
} while(0); } static void ops10(void) { do { JR(!(_P&N_FLAG)); break;

/* BVC */
} while(0); } static void ops50(void) { do { JR(!(_P&V_FLAG)); break;

/* BVS */
} while(0); } static void ops70(void) { do { JR(_P&V_FLAG); break;

//default: printf("Bad %02x at $%04x\n",b1,X.PC);break;
//ifdef moo
/* Here comes the undocumented instructions block.  Note that this implementation
   may be "wrong".  If so, please tell me.
*/

/* AAC */
} while(0); } static void ops0B(void) { do { LD_IM(AND;_P&=~C_FLAG;_P|=_A>>7);
} while(0); } static void ops2B(void) { do { ops0B();

/* AAX */
} while(0); } static void ops87(void) { do { ST_ZP(_A&_X);
} while(0); } static void ops97(void) { do { ST_ZPY(_A&_X);
} while(0); } static void ops8F(void) { do { ST_AB(_A&_X);
} while(0); } static void ops83(void) { do { ST_IX(_A&_X);

/* ARR - ARGH, MATEY! */
} while(0); } static void ops6B(void) { do { {
	     uint8 arrtmp; 
	     LD_IM(AND;_P&=~V_FLAG;_P|=(_A^(_A>>1))&0x40;arrtmp=_A>>7;_A>>=1;_A|=(_P&C_FLAG)<<7;_P&=~C_FLAG;_P|=arrtmp;X_ZN(_A));
	   }
/* ASR */
} while(0); } static void ops4B(void) { do { LD_IM(AND;LSRA);

/* ATX(OAL) Is this(OR with $EE) correct? Blargg did some test
   and found the constant to be OR with is $FF for NES */
} while(0); } static void opsAB(void) { do { LD_IM(_A|=0xFF;AND;_X=_A);

/* AXS */ 
} while(0); } static void opsCB(void) { do { LD_IM(AXS);

/* DCP */
} while(0); } static void opsC7(void) { do { RMW_ZP(DEC;CMP);
} while(0); } static void opsD7(void) { do { RMW_ZPX(DEC;CMP);
} while(0); } static void opsCF(void) { do { RMW_AB(DEC;CMP);
} while(0); } static void opsDF(void) { do { RMW_ABX(DEC;CMP);
} while(0); } static void opsDB(void) { do { RMW_ABY(DEC;CMP);
} while(0); } static void opsC3(void) { do { RMW_IX(DEC;CMP);
} while(0); } static void opsD3(void) { do { RMW_IY(DEC;CMP);

/* ISB */
} while(0); } static void opsE7(void) { do { RMW_ZP(INC;SBC);
} while(0); } static void opsF7(void) { do { RMW_ZPX(INC;SBC);
} while(0); } static void opsEF(void) { do { RMW_AB(INC;SBC);
} while(0); } static void opsFF(void) { do { RMW_ABX(INC;SBC);
} while(0); } static void opsFB(void) { do { RMW_ABY(INC;SBC);
} while(0); } static void opsE3(void) { do { RMW_IX(INC;SBC);
} while(0); } static void opsF3(void) { do { RMW_IY(INC;SBC);

/* DOP */

} while(0); } static void ops04(void) { do { _PC++;break;
} while(0); } static void ops14(void) { do { _PC++;break;
} while(0); } static void ops34(void) { do { _PC++;break;
} while(0); } static void ops44(void) { do { _PC++;break;
} while(0); } static void ops54(void) { do { _PC++;break;
} while(0); } static void ops64(void) { do { _PC++;break;
} while(0); } static void ops74(void) { do { _PC++;break;

} while(0); } static void ops80(void) { do { _PC++;break;
} while(0); } static void ops82(void) { do { _PC++;break;
} while(0); } static void ops89(void) { do { _PC++;break;
} while(0); } static void opsC2(void) { do { _PC++;break;
} while(0); } static void opsD4(void) { do { _PC++;break;
} while(0); } static void opsE2(void) { do { _PC++;break;
} while(0); } static void opsF4(void) { do { _PC++;break;

/* KIL */

} while(0); } static void opsF2(void) { do {ADDCYC(0xFF);
          _jammed=1;
	  _PC--;
	  break;
} while(0); } static void ops02(void) { do { opsF2();
} while(0); } static void ops12(void) { do { opsF2();
} while(0); } static void ops22(void) { do { opsF2();
} while(0); } static void ops32(void) { do { opsF2();
} while(0); } static void ops42(void) { do { opsF2();
} while(0); } static void ops52(void) { do { opsF2();
} while(0); } static void ops62(void) { do { opsF2();
} while(0); } static void ops72(void) { do { opsF2();
} while(0); } static void ops92(void) { do { opsF2();
} while(0); } static void opsB2(void) { do { opsF2();
} while(0); } static void opsD2(void) { do { opsF2();
	
/* LAR */
} while(0); } static void opsBB(void) { do { RMW_ABY(_S&=x;_A=_X=_S;X_ZN(_X));

/* LAX */
} while(0); } static void opsA7(void) { do { LD_ZP(LDA;LDX);
} while(0); } static void opsB7(void) { do { LD_ZPY(LDA;LDX);
} while(0); } static void opsAF(void) { do { LD_AB(LDA;LDX);
} while(0); } static void opsBF(void) { do { LD_ABY(LDA;LDX);
} while(0); } static void opsA3(void) { do { LD_IX(LDA;LDX);
} while(0); } static void opsB3(void) { do { LD_IY(LDA;LDX);

/* NOP */
} while(0); } static void ops1A(void) { do { break;
} while(0); } static void ops3A(void) { do { break;
} while(0); } static void ops5A(void) { do { break;
} while(0); } static void ops7A(void) { do { break;
} while(0); } static void opsDA(void) { do { break;
} while(0); } static void opsFA(void) { do { break;

/* RLA */
} while(0); } static void ops27(void) { do { RMW_ZP(ROL;AND);
} while(0); } static void ops37(void) { do { RMW_ZPX(ROL;AND);
} while(0); } static void ops2F(void) { do { RMW_AB(ROL;AND);
} while(0); } static void ops3F(void) { do { RMW_ABX(ROL;AND);
} while(0); } static void ops3B(void) { do { RMW_ABY(ROL;AND);
} while(0); } static void ops23(void) { do { RMW_IX(ROL;AND);
} while(0); } static void ops33(void) { do { RMW_IY(ROL;AND);

/* RRA */
} while(0); } static void ops67(void) { do { RMW_ZP(ROR;ADC);
} while(0); } static void ops77(void) { do { RMW_ZPX(ROR;ADC);
} while(0); } static void ops6F(void) { do { RMW_AB(ROR;ADC);
} while(0); } static void ops7F(void) { do { RMW_ABX(ROR;ADC);
} while(0); } static void ops7B(void) { do { RMW_ABY(ROR;ADC);
} while(0); } static void ops63(void) { do { RMW_IX(ROR;ADC);
} while(0); } static void ops73(void) { do { RMW_IY(ROR;ADC);

/* SLO */
} while(0); } static void ops07(void) { do { RMW_ZP(ASL;ORA);
} while(0); } static void ops17(void) { do { RMW_ZPX(ASL;ORA);
} while(0); } static void ops0F(void) { do { RMW_AB(ASL;ORA);
} while(0); } static void ops1F(void) { do { RMW_ABX(ASL;ORA);
} while(0); } static void ops1B(void) { do { RMW_ABY(ASL;ORA);
} while(0); } static void ops03(void) { do { RMW_IX(ASL;ORA);
} while(0); } static void ops13(void) { do { RMW_IY(ASL;ORA);

/* SRE */
} while(0); } static void ops47(void) { do { RMW_ZP(LSR;EOR);
} while(0); } static void ops57(void) { do { RMW_ZPX(LSR;EOR);
} while(0); } static void ops4F(void) { do { RMW_AB(LSR;EOR);
} while(0); } static void ops5F(void) { do { RMW_ABX(LSR;EOR);
} while(0); } static void ops5B(void) { do { RMW_ABY(LSR;EOR);
} while(0); } static void ops43(void) { do { RMW_IX(LSR;EOR);
} while(0); } static void ops53(void) { do { RMW_IY(LSR;EOR);

/* AXA - SHA */
} while(0); } static void ops93(void) { do { ST_IY(_A&_X&(((A-_Y)>>8)+1));
} while(0); } static void ops9F(void) { do { ST_ABY(_A&_X&(((A-_Y)>>8)+1));

/* SYA */
} while(0); } static void ops9C(void) { do { ST_ABX(_Y&(((A-_X)>>8)+1));

/* SXA */
} while(0); } static void ops9E(void) { do { ST_ABY(_X&(((A-_Y)>>8)+1));

/* XAS */
} while(0); } static void ops9B(void) { do { _S=_A&_X;ST_ABY(_S& (((A-_Y)>>8)+1) );

/* TOP */
} while(0); } static void ops0C(void) { do { LD_AB(;);
} while(0); } static void opsFC(void) { do { LD_ABX(;);
} while(0); } static void ops1C(void) { do { opsFC();
} while(0); } static void ops3C(void) { do { opsFC();
} while(0); } static void ops5C(void) { do { opsFC();
} while(0); } static void ops7C(void) { do { opsFC();
} while(0); } static void opsDC(void) { do { opsFC();

/* XAA - BIG QUESTION MARK HERE */
} while(0); } static void ops8B(void) { do { _A|=0xEE; _A&=_X; LD_IM(AND);
} while(0); }
//endif

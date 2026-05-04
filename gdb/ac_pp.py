"""************************************************************************
 *                                                                        *
 *  Algorithmic C (tm) Datatypes                                          *
 *                                                                        *
 *  Software Version: 2003.1       *
 *                                                                        *
 *  Release Date    : Tue Feb 10 18:26:09 PST 2026                        *
 *  Release Type    : Production Release                                  *
 *  Release Build   :        *
 *                                                                        *
 *  Copyright 2013-2021 Siemens                                                *
 *                                                                        *
 *                                                                        *
 *                                                                        *
 **************************************************************************
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *      http://www.apache.org/licenses/LICENSE-2.0                        *
 *                                                                        *
 *  Unless required by applicable law or agreed to in writing, software   *
 *  distributed under the License is distributed on an "AS IS" BASIS,     *
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or       *
 *  implied.                                                              *
 *  See the License for the specific language governing permissions and   *
 *  limitations under the License.                                        *
 **************************************************************************
 *                                                                        *
 *  The most recent version of this package is available at github.       *
 *                                                                        *
 ***********************************************************************"""
#
# GDB Pretty Printer for AC Datatypes
#
# Authors: 
#   Lucien Murrey-Pitts
#     First version for ac_int/ac_fixed limited to 64-bits 
#   Andres Takach 
#     Arbitrary-length ac_int, ac_fixed, ac_float, ac_complex  
#
# Requires a GDB that supports Python
#   available from the "stable" GDB releases in 
#   - ftp://sourceware.org/pub/gdb/snapshots/current/
#
# Alternatives ways to import this script in gdb 
# 1. run gdb and specify -ex 'py exec(open("ac_pp.py").read())'
#
# 2. run gdb, and type the command: 
#    py exec(open("ac_pp.py").read())
#
# 3  load it into the .gdbinit file by adding: 
#    py exec(open("ac_pp.py").read())
#

# Pretty print parameters for AC Datatypes
#   set pp-ac-radix {dec, hex, bin}
#   set pp-ac_int-radix {default, dec, hex, bin}
#      default -> defaults to pp-ac-radix setting
#   set pp-ac_fixed-radix {default, dec, hex, bin}
#      default -> defaults to pp-ac-radix setting
#
# Known Limitations:
#   No direct parameter control for ac_float
#     Mantissa/Exponent view affected by ac_fixed/ac_int settings
#   Printing of negative numbers in hex/bin takes the
#     2's complement and represents the number using "-"
#     in front. No setting to get actual bit pattern for them. 

import re
import decimal
import gdb
import sys

if sys.version_info[0] > 2:
    long = int

def real_type (val):
    # Get the type.
    type = val.type

    # If it points to a reference, get the reference.
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target ()

    # Get the unqualified type, stripped of typedefs.
    type = type.unqualified ().strip_typedefs ()

    return type

class pp_iv:
    def __init__(self, val):
        self.val = val
        fields = real_type(val).fields()
        ts = fields[0].name
        m1 = re.match( r'.*ac_private::iv<([0-9]*)>.*', ts )
        if m1==None:
            fields = fields[0].type.fields() 
            ts = fields[0].name
            m1 = re.match( r'.*ac_private::iv<([0-9]*)>.*', ts )
        self.words = long(m1.group(1))

    def to_long(self):
        v = self.val["v"]
        int_val = long(v[self.words-1])
        for word_i in range(self.words-2, -1, -1):
            int_val *= (2 ** 32)
            int_val += long((v[word_i] & 0xffffffff))
        return int_val

class pp_ac_int:
    def __init__(self, val):
        self.val = val
        t_name = real_type(val).__str__()
        m1 = re.match( r'ac_int<([0-9]*),\s*(.*)>$', t_name )
        self.w = int(m1.group(1))

    def to_long(self):
        return pp_iv(self.val).to_long() 

    def to_string_base(self,radix):
        if radix == "dec":
            return str(self.to_long())
        elif radix == "hex":
            hex_len = (self.w + 3)//4 
            hex_str = "{0:=#0{width}x}".format(self.to_long(), width=hex_len+2)
            return hex_str 
        elif radix == "bin":
            return str(bin(self.to_long())) 

    def to_string(self):
        radix = gdb.parameter('pp-ac_int-radix')
        if radix == "default":
           radix = gdb.parameter('pp-ac-radix')
        return self.to_string_base(radix)

class pp_ac_fixed:
    def __init__(self, val):
        self.val = val
        t_name = real_type(val).__str__()
        # when the second argument (I) is negative, for some versions, it is written out in hex!
        m1 = re.match( r'^ac_fixed<([0-9]*),\s*([-]?[x0-9a-fA-F]*),\s*(.*),\s*(.*),\s*(.*)>$', t_name )
        self.w = int(m1.group(1))
        self.i = int(m1.group(2),0)

    def to_decimal(self):
        base_iv = pp_iv(self.val)
        return decimal.Decimal(base_iv.to_long()) * (decimal.Decimal(2) ** (self.i - self.w)) 

    def to_string_base(self,radix):
        if radix == "dec":
            return str(self.to_decimal())
        else:
            if radix == "hex":
               bits = 4
               base_letter = 'x'
            elif radix == "bin":
               bits = 1
               base_letter = 'b'

            lsb = self.i - self.w 
            msb = self.i - 1
            base_lsb = lsb // bits
            base_msb = msb // bits
            if base_lsb > 0:
               base_lsb = 0
            if base_msb < 0:
               base_msb = 0
            base_len = base_msb - base_lsb + 1
            extra_lsb_bits = lsb - bits * base_lsb 
            int_val = pp_iv(self.val).to_long() * (2 ** extra_lsb_bits)
            s = "{0:=#0{width}{base_letter}}".format(int_val, width=base_len+2, base_letter=base_letter)
            if lsb < 0:
               s = s[:base_lsb] + '.' + s[base_lsb:]
            return s

    def to_string(self):
        radix = gdb.parameter('pp-ac_fixed-radix')
        if radix == "default":
           radix = gdb.parameter('pp-ac-radix')
        return self.to_string_base(radix)

class pp_ac_float:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        m = self.val["m"]
        e = self.val["e"]
        m0 = pp_ac_fixed(m)
        e0 = pp_ac_int(e)
        flt = m0.to_decimal() * (decimal.Decimal(2) ** e0.to_long()) 
        return str(flt) + " = " + m0.to_string() + "*2^" + e0.to_string() 

class pp_ac_complex:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        r = self.val["_r"]
        i = self.val["_i"]
        return "(" + str(r) + " + j " + str(i) +")"

def read_slice_r(val, msb):
    p2 = 2 ** msb 
    q = val // p2
    return val - q*p2

def read_slice_l(val, lsb):
    p2 = 2 ** lsb 
    return val // p2

def value_for_ll_array(d, words):
        int_val = long(d[words-1])
        for word_i in range(words-2, -1, -1):
            int_val *= (2 ** 64)
            int_val += long((d[word_i] & 0xffffffffffffffff))
        return int_val

class pp_ac_std_float_helper:
    def __init__(self, d, w, e_w):
        m_w = w - e_w - 1
        exp_bias = (1 << (e_w-1)) - 1
        d_without_sign = read_slice_r(d, w-1)

        self.width = w
        self.e_width = e_w
        self.sign = d < 0
        raw_exp = read_slice_l(d_without_sign, m_w)
        raw_mant = read_slice_r(d_without_sign, m_w)
        self.nan = False
        self.inf = False 
        self.mant = raw_mant
        self.exp = raw_exp - exp_bias
        if raw_exp != (1 << e_w) - 1:
            if raw_exp:
                self.mant = raw_mant + 2 ** m_w
            else:
                self.exp += 1
        else:
            if raw_mant:
                self.nan = True
            else:
                self.inf = True

    def to_decimal(self):
        if self.nan:
            f = decimal.Decimal('NaN')
        elif self.inf:
            f = decimal.Decimal('Infinity')
        else:
            m_w = self.width - self.e_width - 1
            mant_f = self.mant / (decimal.Decimal(2)**m_w)
            f = mant_f * (decimal.Decimal(2) ** self.exp)
        if self.sign:
            f = -f
        return f

    def to_string(self):
        return str(self.to_decimal())

class pp_ac_std_float:
    def __init__(self, val):
        self.val = val
        t_name = real_type(val).__str__()
        m1 = re.match( r'^ac_std_float<([0-9]*),\s*([0-9]*)\s*>$', t_name )
        self.w = int(m1.group(1))
        self.e_w = int(m1.group(2))


    def to_string(self):
        d = pp_iv(self.val["d"]).to_long()
        return pp_ac_std_float_helper(d, self.w, self.e_w).to_string() 


class pp_ac_ieee_float:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        w = int(self.val["width"])
        e_w = int(self.val["e_width"])
        d_field = self.val["d"]
        if real_type(d_field).code == gdb.TYPE_CODE_FLT:
            return str(d_field)
        if w >= 128:
            d = value_for_ll_array(d_field, w // 64)
        else:
            d = long(d_field)
        return pp_ac_std_float_helper(d, w, e_w).to_string() 

class pp_ac_bfloat16:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        width = int(self.val["width"])
        e_width = int(self.val["e_width"])
        d = int(self.val["d"])
        return pp_ac_std_float_helper(d, width, e_width).to_string() 

def ac_lookup_function (val):
    "Look-up and return a pretty-printer that can print val."

    # Get the real type.
    type = real_type(val)

    # Get the type name.    
    typename = type.tag

    if typename == None:
        return None

    # Iterate over local dictionary of types to determine
    # if a printer is registered for that type.  Return an
    # instantiation of the printer if found.
    for function in ac_pretty_printers_dict:
        if function.match (typename):
            return ac_pretty_printers_dict[function] (val)
        
    # Cannot find a pretty printer.  Return None.

    return None

def register_ac_pretty_printers ():
    # Note that we purposely omit the typedef names here.
    # Printer lookup is based on canonical name.
    # However, we do need both tagged and untagged variants, to handle
    # both the C and C++ cases.
    ac_pretty_printers_dict[re.compile ('^class ac_int<.*>$')]   = pp_ac_int
    ac_pretty_printers_dict[re.compile ('^ac_int<.*>$')]   = pp_ac_int

    ac_pretty_printers_dict[re.compile ('^class ac_fixed<.*>$')]   = pp_ac_fixed
    ac_pretty_printers_dict[re.compile ('^ac_fixed<.*>$')]   = pp_ac_fixed

    ac_pretty_printers_dict[re.compile ('^class ac_float<.*>$')]   = pp_ac_float
    ac_pretty_printers_dict[re.compile ('^ac_float<.*>$')]   = pp_ac_float

    ac_pretty_printers_dict[re.compile ('^class ac_complex<.*>$')]   = pp_ac_complex
    ac_pretty_printers_dict[re.compile ('^ac_complex<.*>$')]   = pp_ac_complex

    ac_pretty_printers_dict[re.compile ('^class ac_std_float<.*>$')]   = pp_ac_std_float
    ac_pretty_printers_dict[re.compile ('^ac_std_float<.*>$')]   = pp_ac_std_float

    ac_pretty_printers_dict[re.compile ('^class ac_ieee_float<.*>$')]   = pp_ac_ieee_float
    ac_pretty_printers_dict[re.compile ('^ac_ieee_float<.*>$')]   = pp_ac_ieee_float

    ac_pretty_printers_dict[re.compile ('^class ac::bfloat16$')]   = pp_ac_bfloat16
    ac_pretty_printers_dict[re.compile ('^ac::bfloat16$')]   = pp_ac_bfloat16

class PP_Param_ac_Radix (gdb.Parameter):
    set_doc = "Radix setting for Pretty Print for Algorithmic C Datatypes"
    show_doc = "Radix setting for Pretty Print for Algorithmic C Datatypes"
    def __init__(self):
        super (PP_Param_ac_Radix, self).__init__("pp-ac-radix", gdb.COMMAND_DATA, gdb.PARAM_ENUM, ["dec","hex","bin"]) 
        self.value = "dec" 
    
    def get_set_string(self):
        return self.value

    def get_show_string(self,svalue):
        svalue = self.value
        return svalue 

class PP_Param_ac_int_Radix (gdb.Parameter):
    set_doc = "Radix setting for Pretty Print for ac_int"
    show_doc = "Radix setting for Pretty Print for ac_int"
    def __init__(self):
        super (PP_Param_ac_int_Radix, self).__init__("pp-ac_int-radix", gdb.COMMAND_DATA, gdb.PARAM_ENUM, ["default","dec","hex","bin"]) 
        self.value = "default" 
    
    def get_set_string(self):
        return self.value

    def get_show_string(self,svalue):
        svalue = self.value
        return svalue 

class PP_Param_ac_fixed_Radix (gdb.Parameter):
    set_doc = "Radix setting for Pretty Print for ac_fixed"
    show_doc = "Radix setting for Pretty Print for ac_fixed"
    def __init__(self):
        super (PP_Param_ac_fixed_Radix, self).__init__("pp-ac_fixed-radix", gdb.COMMAND_DATA, gdb.PARAM_ENUM, ["default","dec","hex","bin"]) 
        self.value = "default" 
    
    def get_set_string(self):
        return self.value

    def get_show_string(self,svalue):
        svalue = self.value
        return svalue 
    
class PP_Print_ac_datatypes(gdb.Command):
    set_doc = "Print command for AC Datatypes with radix options ('d', 'x' or 't')."
    show_doc = "Print command for AC Datatypes with radix options ('d', 'x' or 't')."
    def __init__(self):
        super().__init__("acprint", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, from_tty):
        format_ac_int = False
        format_ac_fixed = False
        pradix = 'default'
        args = gdb.string_to_argv(arg)
        if len(args) == 2:
            var = gdb.parse_and_eval(args[1])
            pp = gdb.default_visualizer(var)     
            class_name = type(pp).__name__ 
            if args[0][0] == '/':
                r_val = '' if len(args[0]) != 2 else args[0][1]
                valid_radix = r_val in ['d','x','t']
                format_ac_int   = (class_name == "pp_ac_int") and valid_radix
                format_ac_fixed = (class_name == "pp_ac_fixed") and valid_radix
                if format_ac_int or format_ac_fixed:
                    current_ac_int_radix   = gdb.parameter('pp-ac_int-radix') 
                    current_ac_fixed_radix = gdb.parameter('pp-ac_fixed-radix') 
                    current_radix = current_ac_int_radix if format_ac_int else current_ac_fixed_radix if format_ac_fixed else 'default'
                    if current_radix == 'default':
                        current_radix = gdb.parameter('pp-ac-radix')
                    
                    pradix = 'dec' if r_val == 'd' else 'hex' if r_val == 'x' else 'bin' if r_val == 't' else current_radix

        if format_ac_int:
            gdb.execute(f"set pp-ac_int-radix {pradix}", from_tty=False, to_string=True)
        if format_ac_fixed:
            gdb.execute(f"set pp-ac_fixed-radix {pradix}", from_tty=False, to_string=True)

        gdb.execute(f"print {arg}")

        if format_ac_int:
            gdb.execute(f"set pp-ac_int-radix {current_ac_int_radix}", from_tty=False, to_string=True)
        if format_ac_fixed:
            gdb.execute(f"set pp-ac_fixed-radix {current_ac_fixed_radix}", from_tty=False, to_string=True)

import itertools

_displays = {}
_next_num = itertools.count(1)       

class PP_Display_ac_datatypes(gdb.Command):
    set_doc = "Display command for AC Datatypes with radix options ('d', 'x' or 't')."
    show_doc = "Display command for AC Datatypes with radix options ('d', 'x' or 't')."
    def __init__(self):
        super().__init__("acdisplay", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def invoke(self, arg, from_tty):
        arg = arg.strip()
        if not arg:
            gdb.write("Usage: acdisplay [/fmt] expression\n", gdb.STDERR)
            return

        fmt = None
        if arg.startswith("/"):
            try:
                fmt, expr = arg.split(None, 1)
            except ValueError:
                gdb.write("Missing expression after /fmt\n", gdb.STDERR)
                return
            fmt = "%" + fmt[1:]   # convert "/x" → "%x", "/d" → "%d", etc.
        else:
            expr = arg

        num = next(_next_num)
        _displays[num] = (expr, fmt, True)

        current_ac_int_radix   = gdb.parameter('pp-ac_int-radix') 
        current_ac_fixed_radix = gdb.parameter('pp-ac_fixed-radix')

        value = gdb.parse_and_eval(expr)
        pp = gdb.default_visualizer(value)     
        class_name = type(pp).__name__ 
        if  class_name in {"pp_ac_int", "pp_ac_fixed"}:
            if fmt and fmt[1:] in ['d','x','t']:
                pradix = 'dec' if fmt[1:] == 'd' else 'hex' if fmt[1:] == 'x' else 'bin'
                tmp = pp.to_string_base(pradix)

                gdb.write(f"{num}: {expr} = {tmp}\n")
            else:
                tmp = pp.to_string()
                gdb.write(f"{num}: {expr} = {tmp}\n")
        else:
            if fmt:
                if fmt == '%t':
                    gdb.write(f"{num}: {expr} = {bin(value)}\n")
                else:
                    gdb.execute(f"printf \"{num}: {expr} = {fmt}\\n\", {value}")
            else:
                gdb.write(f"{num}: {expr} = {value}\n")

class PP_Info_display_ac_datatypes(gdb.Command):
    set_doc = "Info display command for AC Datatypes (acdisplay)."
    show_doc = "Info display command for AC Datatypes (acdisplay)."
    def __init__(self):
        super().__init__("info acdisplay", gdb.COMMAND_STATUS)

    def invoke(self, arg, from_tty):
        if not _displays:
            gdb.write("There are no acdisplay expressions.\n")
            return
        gdb.write("Num Enb Expression (Format)\n")
        for num, (expr, fmt, enabled) in sorted(_displays.items()):
            suffix = f" (/{fmt[1:]})" if fmt else ""
            en = 'y' if enabled else 'n'
            gdb.write(f"{num}:   {en}  {expr} {suffix}\n")

class PP_Delete_display_ac_datatypes(gdb.Command):
    set_doc = "Delete command for AC Datatypes (acdisplay)."
    show_doc = "Delete command for AC Datatypes (acdisplay)."
    def __init__(self):
        super().__init__("delete acdisplay", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        if not _displays:
            gdb.write("No acdisplays to delete.\n")
            return
        if arg.strip():
            argv = arg.split()
            nums = []
            for v in argv:
                ranged_v = v.split('-')
                if len(ranged_v) == 2:
                    lower = int(ranged_v[0])
                    upper = int(ranged_v[1])+1
                    for n in range(lower, upper):
                        nums.append(int(n))
                else:
                    nums.append(int(v))
        else:
            nums = list(_displays)
        for n in nums:
            if n in _displays:
                del _displays[n]
            else:
                gdb.write(f"No acdisplay number: {n}\n", gdb.STDERR)


class PP_Enable_display_ac_datatypes(gdb.Command):
    set_doc = "Enable command for AC Datatypes (acdisplay)."
    show_doc = "Enable command for AC Datatypes (acdisplay)."
    def __init__(self):
        super().__init__("enable acdisplay", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        if not _displays:
            gdb.write("No acdisplays to enable.\n")
            return
        if arg.strip():
            argv = arg.split()
            nums = []
            for v in argv:
                ranged_v = v.split('-')
                if len(ranged_v) == 2:
                    lower = int(ranged_v[0])
                    upper = int(ranged_v[1])+1
                    for n in range(lower, upper):
                        nums.append(int(n))
                else:
                    nums.append(int(v))
        else:
            nums = list(_displays)
        for n in nums:
            if n in _displays:
               (ex, f, en) = _displays[n]
               _displays[n] = (ex, f, True)
            else:
                gdb.write(f"No acdisplay number: {n}\n", gdb.STDERR)

class PP_Disable_display_ac_datatypes(gdb.Command):
    set_doc = "Disable command for AC Datatypes (acdisplay)."
    show_doc = "Disable command for AC Datatypes (acdisplay)."
    def __init__(self):
        super().__init__("disable acdisplay", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        if not _displays:
            gdb.write("No acdisplays to disable.\n")
            return
        if arg.strip():
            argv = arg.split()
            nums = []
            for v in argv:
                ranged_v = v.split('-')
                if len(ranged_v) == 2:
                    lower = int(ranged_v[0])
                    upper = int(ranged_v[1])+1
                    for n in range(lower, upper):
                        nums.append(int(n))
                else:
                    nums.append(int(v))
        else:
            nums = list(_displays)
        for n in nums:
            if n in _displays:
               (ex, f, en) = _displays[n]
               _displays[n] = (ex, f, False)
            else:
                gdb.write(f"No acdisplay number: {n}\n", gdb.STDERR)

def _acdisplay_on_stop(event):
    # Called *after* GDB stops but *before* the prompt re-appears
    for num, (expr, fmt, enabled) in sorted(_displays.items()):
        try:
            if enabled:
                value = gdb.parse_and_eval(expr)
                pp = gdb.default_visualizer(value)     
                class_name = type(pp).__name__ 
                if  class_name in {"pp_ac_int", "pp_ac_fixed"}:
                    if fmt and fmt[1:] in ['d','x','t']:
                        pradix = 'dec' if fmt[1:] == 'd' else 'hex' if fmt[1:] == 'x' else 'bin'
                        tmp = pp.to_string_base(pradix)

                        gdb.write(f"{num}: /{fmt[1:]} {expr} = {tmp}\n")
                    else:
                        tmp = pp.to_string()
                        gdb.write(f"{num}: {expr} = {tmp}\n")
                else:
                    if fmt:
                        if fmt == '%t':
                            gdb.write(f"{num}: /{fmt[1:]} {expr} = {bin(value)}\n")
                        else:
                            gdb.execute(f"printf \"{num}: /{fmt[1:]} {expr} = {fmt}\\n\", {value}")
                    else:
                        gdb.write(f"{num}: {expr} = {value}\n")
        except Exception as exc:
            gdb.write(f"{num}: {expr} -> <error: {exc}>\n", gdb.STDERR)

if __name__ == "__main__":
    ac_pretty_printers_dict = {}
    register_ac_pretty_printers ()
    gdb.pretty_printers.append (ac_lookup_function)
    PP_Param_ac_Radix()
    PP_Param_ac_int_Radix()
    PP_Param_ac_fixed_Radix()
    PP_Print_ac_datatypes() 
    PP_Display_ac_datatypes()
    PP_Info_display_ac_datatypes()
    PP_Delete_display_ac_datatypes()
    PP_Enable_display_ac_datatypes()
    PP_Disable_display_ac_datatypes()
    gdb.events.stop.connect(_acdisplay_on_stop)

/*
 *  yosys -- Yosys Open SYnthesis Suite
 *
 *  Copyright (C) 2012  Claire Xenia Wolf <claire@yosyshq.com>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include "kernel/rtlil.h"
#include "kernel/register.h"
#include "kernel/sigtools.h"
#include "kernel/celltypes.h"
#include "kernel/cellaigs.h"
#include "kernel/log.h"
#include "sexpression.hpp"
#include <string>

USING_YOSYS_NAMESPACE
PRIVATE_NAMESPACE_BEGIN

struct SexprWriter
{
	std::ostream &f;
	bool use_selection;

	Design *design;
	Module *module;

	SigMap sigmap;
	int sigidcounter;
	dict<SigBit, string> sigids;
	pool<Aig> aig_models;

	SexprWriter(std::ostream &f, bool use_selection) :
			f(f), use_selection(use_selection) {}

	string get_string(string str)
	{
		string newstr = "\"";
		for (char c : str) {
			if (c == '\\')
				newstr += "\\\\";
			else if (c == '"')
				newstr += "\\\"";
			else if (c == '\b')
				newstr += "\\b";
			else if (c == '\f')
				newstr += "\\f";
			else if (c == '\n')
				newstr += "\\n";
			else if (c == '\r')
				newstr += "\\r";
			else if (c == '\t')
				newstr += "\\t";
			else if (c < 0x20)
				newstr += stringf("\\u%04X", c);
			else
				newstr += c;
		}
		return newstr + "\"";
	}

	string get_name(IdString name)
	{
		return get_string(RTLIL::unescape_id(name));
	}

	string get_bits(SigSpec sig)
	{
		bool first = true;
		string str = "[";
		for (auto bit : sigmap(sig)) {
			str += first ? " " : ", ";
			first = false;
			if (sigids.count(bit) == 0) {
				string &s = sigids[bit];
				if (bit.wire == nullptr) {
					if (bit == State::S0) s = "\"0\"";
					else if (bit == State::S1) s = "\"1\"";
					else if (bit == State::Sz) s = "\"z\"";
					else s = "\"x\"";
				} else
					s = stringf("%d", sigidcounter++);
			}
			str += sigids[bit];
		}
		return str + " ]";
	}

	void write_parameter_value(const Const &value)
	{
		if ((value.flags & RTLIL::ConstFlags::CONST_FLAG_STRING) != 0) {
			string str = value.decode_string();
			int state = 0;
			for (char c : str) {
				if (state == 0) {
					if (c == '0' || c == '1' || c == 'x' || c == 'z')
						state = 0;
					else if (c == ' ')
						state = 1;
					else
						state = 2;
				} else if (state == 1 && c != ' ')
					state = 2;
			}
			if (state < 2)
				str += " ";
			f << get_string(str);
		} else if (false) {//compat_int_mode && GetSize(value) <= 32 && value.is_fully_def()) {
			if ((value.flags & RTLIL::ConstFlags::CONST_FLAG_SIGNED) != 0)
				f << stringf("%d", value.as_int());
			else
				f << stringf("%u", value.as_int());
		} else {
			f << get_string(value.as_string());
		}
	}

	void write_parameters(const dict<IdString, Const> &parameters, bool for_module=false)
	{
		bool first = true;
		for (auto &param : parameters) {
			f << stringf("%s\n", first ? "" : ",");
			f << stringf("        %s%s: ", for_module ? "" : "    ", get_name(param.first).c_str());
			write_parameter_value(param.second);
			first = false;
		}
	}

	void write_module(Module *module_)
	{
		module = module_;
		log_assert(module->design == design);
		sigmap.set(module);
		sigids.clear();

		// reserve 0 and 1 to avoid confusion with "0" and "1"
		sigidcounter = 2;

		if (module->has_processes()) {
			log_error("Module %s contains processes, which are not supported by S-Expression backend (run `proc` first).\n", log_id(module));
		}

		f << stringf("    %s: {\n", get_name(module->name).c_str());

		f << stringf("      \"attributes\": {");
		write_parameters(module->attributes, /*for_module=*/true);
		f << stringf("\n      },\n");

		if (module->parameter_default_values.size()) {
			f << stringf("      \"parameter_default_values\": {");
			write_parameters(module->parameter_default_values, /*for_module=*/true);
			f << stringf("\n      },\n");
		}

		f << stringf("      \"ports\": {");
		bool first = true;
		for (auto n : module->ports) {
			Wire *w = module->wire(n);
			if (use_selection && !module->selected(w))
				continue;
			f << stringf("%s\n", first ? "" : ",");
			f << stringf("        %s: {\n", get_name(n).c_str());
			f << stringf("          \"direction\": \"%s\",\n", w->port_input ? w->port_output ? "inout" : "input" : "output");
			if (w->start_offset)
				f << stringf("          \"offset\": %d,\n", w->start_offset);
			if (w->upto)
				f << stringf("          \"upto\": 1,\n");
			if (w->is_signed)
				f << stringf("          \"signed\": %d,\n", w->is_signed);
			f << stringf("          \"bits\": %s\n", get_bits(w).c_str());
			f << stringf("        }");
			first = false;
		}
		f << stringf("\n      },\n");

		f << stringf("      \"cells\": {");
		first = true;
		for (auto c : module->cells()) {
			if (use_selection && !module->selected(c))
				continue;
			// Eventually we will want to emit $scopeinfo, but currently this
			// will break S-Expression netlist consumers like nextpnr
			if (c->type == ID($scopeinfo))
				continue;
			f << stringf("%s\n", first ? "" : ",");
			f << stringf("        %s: {\n", get_name(c->name).c_str());
			f << stringf("          \"hide_name\": %s,\n", c->name[0] == '$' ? "1" : "0");
			f << stringf("          \"type\": %s,\n", get_name(c->type).c_str());
			if (false){ //aig_mode) {
				Aig aig(c);
				if (!aig.name.empty()) {
					f << stringf("          \"model\": \"%s\",\n", aig.name.c_str());
					aig_models.insert(aig);
				}
			}
			f << stringf("          \"parameters\": {");
			write_parameters(c->parameters);
			f << stringf("\n          },\n");
			f << stringf("          \"attributes\": {");
			write_parameters(c->attributes);
			f << stringf("\n          },\n");
			if (c->known()) {
				f << stringf("          \"port_directions\": {");
				bool first2 = true;
				for (auto &conn : c->connections()) {
					string direction = "output";
					if (c->input(conn.first))
						direction = c->output(conn.first) ? "inout" : "input";
					f << stringf("%s\n", first2 ? "" : ",");
					f << stringf("            %s: \"%s\"", get_name(conn.first).c_str(), direction.c_str());
					first2 = false;
				}
				f << stringf("\n          },\n");
			}
			f << stringf("          \"connections\": {");
			bool first2 = true;
			for (auto &conn : c->connections()) {
				f << stringf("%s\n", first2 ? "" : ",");
				f << stringf("            %s: %s", get_name(conn.first).c_str(), get_bits(conn.second).c_str());
				first2 = false;
			}
			f << stringf("\n          }\n");
			f << stringf("        }");
			first = false;
		}
		f << stringf("\n      },\n");

		if (!module->memories.empty()) {
			f << stringf("      \"memories\": {");
			first = true;
			for (auto &it : module->memories) {
				if (use_selection && !module->selected(it.second))
					continue;
				f << stringf("%s\n", first ? "" : ",");
				f << stringf("        %s: {\n", get_name(it.second->name).c_str());
				f << stringf("          \"hide_name\": %s,\n", it.second->name[0] == '$' ? "1" : "0");
				f << stringf("          \"attributes\": {");
				write_parameters(it.second->attributes);
				f << stringf("\n          },\n");
				f << stringf("          \"width\": %d,\n", it.second->width);
				f << stringf("          \"start_offset\": %d,\n", it.second->start_offset);
				f << stringf("          \"size\": %d\n", it.second->size);
				f << stringf("        }");
				first = false;
			}
			f << stringf("\n      },\n");
		}

		f << stringf("      \"netnames\": {");
		first = true;
		for (auto w : module->wires()) {
			if (use_selection && !module->selected(w))
				continue;
			f << stringf("%s\n", first ? "" : ",");
			f << stringf("        %s: {\n", get_name(w->name).c_str());
			f << stringf("          \"hide_name\": %s,\n", w->name[0] == '$' ? "1" : "0");
			f << stringf("          \"bits\": %s,\n", get_bits(w).c_str());
			if (w->start_offset)
				f << stringf("          \"offset\": %d,\n", w->start_offset);
			if (w->upto)
				f << stringf("          \"upto\": 1,\n");
			if (w->is_signed)
				f << stringf("          \"signed\": %d,\n", w->is_signed);
			f << stringf("          \"attributes\": {");
			write_parameters(w->attributes);
			f << stringf("\n          }\n");
			f << stringf("        }");
			first = false;
		}
		f << stringf("\n      }\n");

		f << stringf("    }");
	}

	void write_design(Design *design_)
	{
		design = design_;
		design->sort();

		auto top = cons(token("kicad_pcb"), cons(cons(token("version"),20221018L

  #if 0
		f << stringf("{\n");
		f << stringf("  \"creator\": %s,\n", get_string(yosys_version_str).c_str());
		f << stringf("  \"modules\": {\n");
		vector<Module*> modules = use_selection ? design->selected_modules() : design->modules();
		bool first_module = true;
		for (auto mod : modules) {
			if (!first_module)
				f << stringf(",\n");
			write_module(mod);
			first_module = false;
		}
		f << stringf("\n  }");
		if (!aig_models.empty()) {
			f << stringf(",\n  \"models\": {\n");
			bool first_model = true;
			for (auto &aig : aig_models) {
				if (!first_model)
					f << stringf(",\n");
				f << stringf("    \"%s\": [\n", aig.name.c_str());
				int node_idx = 0;
				for (auto &node : aig.nodes) {
					if (node_idx != 0)
						f << stringf(",\n");
					f << stringf("      /* %3d */ [ ", node_idx);
					if (node.portbit >= 0)
						f << stringf("\"%sport\", \"%s\", %d", node.inverter ? "n" : "",
								log_id(node.portname), node.portbit);
					else if (node.left_parent < 0 && node.right_parent < 0)
						f << stringf("\"%s\"", node.inverter ? "true" : "false");
					else
						f << stringf("\"%s\", %d, %d", node.inverter ? "nand" : "and", node.left_parent, node.right_parent);
					for (auto &op : node.outports)
						f << stringf(", \"%s\", %d", log_id(op.first), op.second);
					f << stringf(" ]");
					node_idx++;
				}
				f << stringf("\n    ]");
				first_model = false;
			}
			f << stringf("\n  }");
		}
		f << stringf("\n}\n");
		#endif
	}
};

struct SexprBackend : public Backend {
	SexprBackend() : Backend("sexpr", "write design to a s-expression file") { }
	void help() override
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		log("\n");
		log("    write_sexpr [options] [filename]\n");
		log("\n");
		log("Write a sexpr netlist of the current design.\n");
		log("\n");
	}

	void execute(std::ostream *&f, std::string filename, std::vector<std::string> args, RTLIL::Design *design) override
	{
		size_t argidx = 1;
		extra_args(f, filename, args, argidx);

		log_header(design, "Executing S-Expression backend.\n");

		SexprWriter sexpr_writer(*f, false);
		sexpr_writer.write_design(design);
	}
} SexprBackend;

struct SexprPass : public Pass {
	SexprPass() : Pass("sexpr", "write design in S-Expression format") { }
	void help() override
	{
		//   |---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|---v---|
		log("\n");
		log("    sexpr [options] [selection]\n");
		log("\n");
		log("Write a S-Expression netlist of all selected objects.\n");
		log("\n");
		log("    -o <filename>\n");
		log("        write to the specified file.\n");
		log("See 'help write_sexpr' for a description of the S-Expression format used.\n");
		log("\n");
	}
	void execute(std::vector<std::string> args, RTLIL::Design *design) override
	{
		std::string filename;

		size_t argidx;
		for (argidx = 1; argidx < args.size(); argidx++)
		{
			if (args[argidx] == "-o" && argidx+1 < args.size()) {
				filename = args[++argidx];
				continue;
			}
			break;
		}
		extra_args(args, argidx, design);

		std::ostream *f;
		std::stringstream buf;
		bool empty = filename.empty();

		if (!empty) {
			rewrite_filename(filename);
			std::ofstream *ff = new std::ofstream;
			ff->open(filename.c_str(), std::ofstream::trunc);
			if (ff->fail()) {
				delete ff;
				log_error("Can't open file `%s' for writing: %s\n", filename.c_str(), strerror(errno));
			}
			f = ff;
		} else {
			f = &buf;
		}

		SexprWriter sexpr_writer(*f, true);
		sexpr_writer.write_design(design);

		if (!empty) {
			delete f;
		} else {
			log("%s", buf.str().c_str());
		}
	}
} SexprPass;

PRIVATE_NAMESPACE_END

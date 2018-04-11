// -*- Mode: C++ -*-
//
// Copyright (C) 2017-2018 Red Hat, Inc.
//
// This file is part of the GNU Application Binary Interface Generic
// Analysis and Instrumentation Library (libabigail).  This library is
// free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the
// Free Software Foundation; either version 3, or (at your option) any
// later version.

// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this program; see the file COPYING-LGPLV3.  If
// not, see <http://www.gnu.org/licenses/>.
//
// Author: Dodji Seketeli


/// @file
///
/// This is the implementation of the
/// abigail::comparison::default_reporter type.

#include "abg-comparison-priv.h"
#include "abg-reporter.h"
#include "abg-reporter-priv.h"

namespace abigail
{
namespace comparison
{

/// Ouputs a report of the differences between of the two type_decl
/// involved in the @ref type_decl_diff.
///
/// @param d the @ref type_decl_diff to consider.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the string to use for indentatino indent.
void
default_reporter::report(const type_decl_diff& d,
			 ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  type_decl_sptr f = d.first_type_decl(), s = d.second_type_decl();

  string name = f->get_pretty_representation();

  bool n = report_name_size_and_alignment_changes(f, s, d.context(),
						  out, indent,
						  /*new line=*/false);

  if (f->get_visibility() != s->get_visibility())
    {
      if (n)
	out << "\n";
      out << indent
	  << "visibility changed from '"
	  << f->get_visibility() << "' to '" << s->get_visibility();
      n = true;
    }

  if (f->get_linkage_name() != s->get_linkage_name())
    {
      if (n)
	out << "\n";
      out << indent
	  << "mangled name changed from '"
	  << f->get_linkage_name() << "' to "
	  << s->get_linkage_name();
      n = true;
    }

  if (n)
    out << "\n";
}

/// Report the differences between the two enums.
///
/// @param d the enum diff to consider.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
default_reporter::report(const enum_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  string name = d.first_enum()->get_pretty_representation();

  enum_type_decl_sptr first = d.first_enum(), second = d.second_enum();

  if (report_name_size_and_alignment_changes(first, second, d.context(),
					     out, indent,
					     /*start_with_num_line=*/false))
    out << "\n";
  maybe_report_diff_for_member(first, second, d.context(), out, indent);

  //underlying type
  d.underlying_type_diff()->report(out, indent);

  //report deletions/insertions/change of enumerators
  unsigned numdels = d.deleted_enumerators().size();
  unsigned numins = d.inserted_enumerators().size();
  unsigned numchanges = d.changed_enumerators().size();

  if (numdels)
    {
      report_mem_header(out, numdels, 0, del_kind, "enumerator", indent);
      enum_type_decl::enumerators sorted_deleted_enumerators;
      sort_enumerators(d.deleted_enumerators(), sorted_deleted_enumerators);
      for (enum_type_decl::enumerators::const_iterator i =
	     sorted_deleted_enumerators.begin();
	   i != sorted_deleted_enumerators.end();
	   ++i)
	{
	  if (i != sorted_deleted_enumerators.begin())
	    out << "\n";
	  out << indent
	      << "  '"
	      << i->get_qualified_name()
	      << "' value '"
	      << i->get_value()
	      << "'";
	}
      out << "\n\n";
    }
  if (numins)
    {
      report_mem_header(out, numins, 0, ins_kind, "enumerator", indent);
      enum_type_decl::enumerators sorted_inserted_enumerators;
      sort_enumerators(d.inserted_enumerators(), sorted_inserted_enumerators);
      for (enum_type_decl::enumerators::const_iterator i =
	     sorted_inserted_enumerators.begin();
	   i != sorted_inserted_enumerators.end();
	   ++i)
	{
	  if (i != sorted_inserted_enumerators.begin())
	    out << "\n";
	  out << indent
	      << "  '"
	      << i->get_qualified_name()
	      << "' value '"
	      << i->get_value()
	      << "'";
	}
      out << "\n\n";
    }
  if (numchanges)
    {
      report_mem_header(out, numchanges, 0, change_kind, "enumerator", indent);
      changed_enumerators_type sorted_changed_enumerators;
      sort_changed_enumerators(d.changed_enumerators(),
			       sorted_changed_enumerators);
      for (changed_enumerators_type::const_iterator i =
	     sorted_changed_enumerators.begin();
	   i != sorted_changed_enumerators.end();
	   ++i)
	{
	  if (i != sorted_changed_enumerators.begin())
	    out << "\n";
	  out << indent
	      << "  '"
	      << i->first.get_qualified_name()
	      << "' from value '"
	      << i->first.get_value() << "' to '"
	      << i->second.get_value() << "'";
	  report_loc_info(second, *d.context(), out);
	}
      out << "\n\n";
    }

  if (d.context()->show_leaf_changes_only())
    maybe_report_interfaces_impacted_by_diff(&d, out, indent,
					     /*new_line_prefix=*/false);
}

/// For a @ref typedef_diff node, report the changes that are local.
///
/// @param d the @ref typedef_diff node to consider.
///
/// @param out the output stream to report to.
///
/// @param indent the white space string to use for indentation.
///
/// @return true iff the caller needs to emit a newline to the output
/// stream before emitting anything else.
bool
default_reporter::report_local_typedef_changes(const typedef_diff &d,
					       ostream& out,
					       const string& indent) const
{
  if (!d.to_be_reported())
    return false;

  bool emit_nl = false;
  typedef_decl_sptr f = d.first_typedef_decl(), s = d.second_typedef_decl();

  maybe_report_diff_for_member(f, s, d.context(), out, indent);

  if ((filtering::has_harmless_name_change(f, s)
       && ((d.context()->get_allowed_category()
	    & HARMLESS_DECL_NAME_CHANGE_CATEGORY)
	   || d.context()->show_leaf_changes_only()))
      || f->get_qualified_name() != s->get_qualified_name())
    {
      out << indent << "typedef name changed from "
	  << f->get_qualified_name()
          << " to "
	  << s->get_qualified_name();
      report_loc_info(s, *d.context(), out);
      out << "\n";
      emit_nl = true;
    }

  return emit_nl;
}

/// Reports the difference between the two subjects of the diff in a
/// serialized form.
///
/// @param d @ref typedef_diff node to consider.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the indentation string to use.
void
default_reporter::report(const typedef_diff& d,
			 ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  typedef_decl_sptr f = d.first_typedef_decl(), s = d.second_typedef_decl();
  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(f, s);

  bool emit_nl = report_local_typedef_changes(d, out, indent);

  diff_sptr dif = d.underlying_type_diff();
  if (dif && dif->to_be_reported())
    {
      RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif, "underlying type");
      out << indent
	  << "underlying type '"
	  << dif->first_subject()->get_pretty_representation() << "'";
      report_loc_info(dif->second_subject(), *d.context(), out);
      out << " changed:\n";
      dif->report(out, indent + "  ");
      emit_nl = false;
    }

  if (emit_nl)
    out << "\n";
}

/// For a @ref qualified_type_diff node, report the changes that are
/// local.
///
/// @param d the @ref qualified_type_diff node to consider.
///
/// @param out the output stream to emit the report to.
///
/// @param indent the white string to use for indentation.
///
/// @return true iff a local change has been emitted.  In this case,
/// the local change is a name change.
bool
default_reporter::report_local_qualified_type_changes(const qualified_type_diff& d,
						      ostream& out,
						      const string& indent) const
{
  if (!d.to_be_reported())
    return false;

  string fname = d.first_qualified_type()->get_pretty_representation(),
    sname = d.second_qualified_type()->get_pretty_representation();

  if (fname != sname)
    {
      out << indent << "'" << fname << "' changed to '" << sname << "'\n";
      return true;
    }
  return false;
}

/// Report a @ref qualified_type_diff in a serialized form.
///
/// @param d the @ref qualified_type_diff node to consider.
///
/// @param out the output stream to serialize to.
///
/// @param indent the string to use to indent the lines of the report.
void
default_reporter::report(const qualified_type_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(d.first_qualified_type(),
						   d.second_qualified_type());

  if (report_local_qualified_type_changes(d, out, indent))
    // The local change was emitted and it's a name change.  If the
    // type name changed, the it means the type changed altogether.
    // It makes a little sense to detail the changes in extenso here.
    return;

  diff_sptr dif = d.leaf_underlying_type_diff();
  assert(dif);
  assert(dif->to_be_reported());
  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif,
						    "unqualified "
						    "underlying type");

  string fltname = dif->first_subject()->get_pretty_representation();
  out << indent << "in unqualified underlying type '" << fltname << "'";
  report_loc_info(dif->second_subject(), *d.context(), out);
  out << ":\n";
  dif->report(out, indent + "  ");
}

/// Report the @ref pointer_diff in a serialized form.
///
/// @param d the @ref pointer_diff node to consider.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
default_reporter::report(const pointer_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  if (diff_sptr dif = d.underlying_type_diff())
    {
      RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif, "pointed to type");
      string repr = dif->first_subject()
	? dif->first_subject()->get_pretty_representation()
	: string("void");

      out << indent
	  << "in pointed to type '" <<  repr << "'";
      report_loc_info(dif->second_subject(), *d.context(), out);
      out << ":\n";
      dif->report(out, indent + "  ");
    }
}

/// For a @reference_diff node, report the local changes carried by
/// the diff node.
///
/// @param d the @reference_diff node to consider.
///
/// @param out the output stream to report to.
///
/// @param indent the white space indentation to use in the report.
void
default_reporter::report_local_reference_type_changes(const reference_diff& d,
						      ostream& out,
						      const string& indent) const
{
  if (!d.to_be_reported())
    return;

  reference_type_def_sptr f = d.first_reference(), s = d.second_reference();
  assert(f && s);


  if (f->is_lvalue() != s->is_lvalue())
    {
      string f_repr = f->get_pretty_representation(),
	s_repr = s->get_pretty_representation();

      out << indent;
      if (f->is_lvalue())
	out << "lvalue reference type '" << f_repr
	    << " became an rvalue reference type: '"
	    << s_repr
	    << "'";
      else
	out << "rvalue reference type '" << f_repr
	    << " became an lvalue reference type: '"
	    << s_repr
	    << "'\n";
    }
}

/// Report a @ref reference_diff in a serialized form.
///
/// @param d the @ref reference_diff node to consider.
///
/// @param out the output stream to serialize the dif to.
///
/// @param indent the string to use for indenting the report.
void
default_reporter::report(const reference_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  report_local_reference_type_changes(d, out, indent);

  if (diff_sptr dif = d.underlying_type_diff())
    {
      RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif, "referenced type");

      out << indent
	  << "in referenced type '"
	  << dif->first_subject()->get_pretty_representation() << "'";
      report_loc_info(dif->second_subject(), *d.context(), out);
      out << ":\n";
      dif->report(out, indent + "  ");
    }
}

/// Emit a textual report about the a @ref fn_parm_diff instance.
///
/// @param d the @ref fn_parm_diff to consider.
///
/// @param out the output stream to emit the textual report to.
///
/// @param indent the indentation string to use in the report.
void
default_reporter::report(const fn_parm_diff& d, ostream& out,
			 const string& indent) const
{
  function_decl::parameter_sptr f = d.first_parameter(),
    s = d.second_parameter();

  // either the parameter has a sub-type change (if its type name
  // hasn't changed) or it has a "grey" change (that is, a change that
  // changes his type name w/o changing the signature of the
  // function).
  bool has_sub_type_change =
    type_has_sub_type_changes(d.first_parameter()->get_type(),
			      d.second_parameter()->get_type());

  if (d.to_be_reported())
    {
      assert(d.type_diff() && d.type_diff()->to_be_reported());
      out << indent
	  << "parameter " << f->get_index();
      report_loc_info(f, *d.context(), out);
      out << " of type '"
	  << f->get_type_pretty_representation();

      if (has_sub_type_change)
	out << "' has sub-type changes:\n";
      else
	out << "' changed:\n";

      d.type_diff()->report(out, indent + "  ");
    }
}

/// For a @ref function_type_diff node, report the local changes
/// carried by the diff node.
///
/// @param d the @ref function_type_diff node to consider.
///
/// @param out the output stream to report to.
///
/// @param indent the white space indentation string to use.
void
default_reporter::report_local_function_type_changes(const function_type_diff& d,
						     ostream& out,
						     const string& indent) const

{
  if (!d.to_be_reported())
    return;

  function_type_sptr fft = d.first_function_type();
  function_type_sptr sft = d.second_function_type();

  diff_context_sptr ctxt = d.context();

  // Report about the size of the function address
  if (fft->get_size_in_bits() != sft->get_size_in_bits())
    {
      out << indent << "address size of function changed from "
	  << fft->get_size_in_bits()
	  << " bits to "
	  << sft->get_size_in_bits()
	  << " bits\n";
    }

  // Report about the alignment of the function address
  if (fft->get_alignment_in_bits()
      != sft->get_alignment_in_bits())
    {
      out << indent << "address alignment of function changed from "
	  << fft->get_alignment_in_bits()
	  << " bits to "
	  << sft->get_alignment_in_bits()
	  << " bits\n";
    }

  // Hmmh, the above was quick.  Now report about function parameters;
  // this shouldn't be as straightforward.


  // Report about the parameters that got removed.
  bool emitted = false;
  for (vector<function_decl::parameter_sptr>::const_iterator i =
	 d.priv_->sorted_deleted_parms_.begin();
       i != d.priv_->sorted_deleted_parms_.end();
       ++i)
    {
      out << indent << "parameter " << (*i)->get_index()
	  << " of type '" << (*i)->get_type_pretty_representation()
	  << "' was removed\n";
      emitted = true;
    }
  if (emitted)
    out << "\n";

  // Report about the parameters that got added
  emitted = false;
  for (vector<function_decl::parameter_sptr>::const_iterator i =
	 d.priv_->sorted_added_parms_.begin();
       i != d.priv_->sorted_added_parms_.end();
       ++i)
    {
      out << indent << "parameter " << (*i)->get_index()
	  << " of type '" << (*i)->get_type_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }

  if (emitted)
    out << "\n";
}

/// Build and emit a textual report about a @ref function_type_diff.
///
/// @param d the @ref function_type_diff to consider.
///
/// @param out the output stream.
///
/// @param indent the indentation string to use.
void
default_reporter::report(const function_type_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  function_type_sptr fft = d.first_function_type();
  function_type_sptr sft = d.second_function_type();

  diff_context_sptr ctxt = d.context();
  corpus_sptr fc = ctxt->get_first_corpus();
  corpus_sptr sc = ctxt->get_second_corpus();

  // Report about return type differences.
  if (d.priv_->return_type_diff_
      && d.priv_->return_type_diff_->to_be_reported())
    {
      out << indent << "return type changed:\n";
      d.priv_->return_type_diff_->report(out, indent + "  ");
    }

  // Report about the parameter types that have changed sub-types.
  for (vector<fn_parm_diff_sptr>::const_iterator i =
	 d.priv_->sorted_subtype_changed_parms_.begin();
       i != d.priv_->sorted_subtype_changed_parms_.end();
       ++i)
    {
      diff_sptr dif = *i;
      if (dif && dif->to_be_reported())
	dif->report(out, indent);
    }

  report_local_function_type_changes(d, out, indent);

}

/// Report a @ref array_diff in a serialized form.
///
/// @param d the @ref array_diff to consider.
///
/// @param out the output stream to serialize the dif to.
///
/// @param indent the string to use for indenting the report.
void
default_reporter::report(const array_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  string name = d.first_array()->get_pretty_representation();
  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER3(d.first_array(),
						    d.second_array(),
						    "array type");

  diff_sptr dif = d.element_type_diff();
  if (dif->to_be_reported())
    {
      string fn = ir::get_pretty_representation(is_type(dif->first_subject()));
      // report array element type changes
      out << indent << "array element type '"
	  << fn << "' changed: \n";
      dif->report(out, indent + "  ");
    }

  report_name_size_and_alignment_changes(d.first_array(),
					 d.second_array(),
					 d.context(),
					 out, indent,
					 /*new line=*/false);
  report_loc_info(d.second_array(), *d.context(), out);
}

/// Generates a report for an intance of @ref base_diff.
///
/// @param d the @ref base_diff to consider.
///
/// @param out the output stream to send the report to.
///
/// @param indent the string to use for indentation.
void
default_reporter::report(const base_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  class_decl::base_spec_sptr f = d.first_base(), s = d.second_base();
  string repr = f->get_base_class()->get_pretty_representation();
  bool emitted = false;

  if (f->get_is_static() != s->get_is_static())
    {
      if (f->get_is_static())
	out << indent << "is no more static";
      else
	out << indent << "now becomes static";
      emitted = true;
    }

  if ((d.context()->get_allowed_category() & ACCESS_CHANGE_CATEGORY)
      && (f->get_access_specifier() != s->get_access_specifier()))
    {
      if (emitted)
	out << ", ";

      out << "has access changed from '"
	  << f->get_access_specifier()
	  << "' to '"
	  << s->get_access_specifier()
	  << "'";

      emitted = true;
    }

  if (class_diff_sptr dif = d.get_underlying_class_diff())
    {
      if (dif->to_be_reported())
	{
	  if (emitted)
	    out << "\n";
	  dif->report(out, indent);
	}
    }
}

/// Report the changes carried by a @ref scope_diff.
///
/// @param d the @ref scope_diff to consider.
///
/// @param out the out stream to report the changes to.
///
/// @param indent the string to use for indentation.
void
default_reporter::report(const scope_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  // Report changed types.
  unsigned num_changed_types = d.changed_types().size();
  if (num_changed_types == 0)
    ;
  else if (num_changed_types == 1)
    out << indent << "1 changed type:\n";
  else
    out << indent << num_changed_types << " changed types:\n";

  for (diff_sptrs_type::const_iterator dif = d.changed_types().begin();
       dif != d.changed_types().end();
       ++dif)
    {
      if (!*dif)
	continue;

      out << indent << "  '"
	  << (*dif)->first_subject()->get_pretty_representation()
	  << "' changed:\n";
      (*dif)->report(out, indent + "    ");
    }

  // Report changed decls
  unsigned num_changed_decls = d.changed_decls().size();
  if (num_changed_decls == 0)
    ;
  else if (num_changed_decls == 1)
    out << indent << "1 changed declaration:\n";
  else
    out << indent << num_changed_decls << " changed declarations:\n";

  for (diff_sptrs_type::const_iterator dif= d.changed_decls().begin();
       dif != d.changed_decls().end ();
       ++dif)
    {
      if (!*dif)
	continue;

      out << indent << "  '"
	  << (*dif)->first_subject()->get_pretty_representation()
          << "' was changed to '"
          << (*dif)->second_subject()->get_pretty_representation() << "'";
      report_loc_info((*dif)->second_subject(), *d.context(), out);
      out << ":\n";

      (*dif)->report(out, indent + "    ");
    }

  // Report removed types/decls
  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->deleted_types_.begin();
       i != d.priv_->deleted_types_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";

  if (d.priv_->deleted_types_.size())
    out << "\n";

  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->deleted_decls_.begin();
       i != d.priv_->deleted_decls_.end();
       ++i)
    out << indent
	<< "  '"
	<< i->second->get_pretty_representation()
	<< "' was removed\n";

  if (d.priv_->deleted_decls_.size())
    out << "\n";

  // Report added types/decls
  bool emitted = false;
  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->inserted_types_.begin();
       i != d.priv_->inserted_types_.end();
       ++i)
    {
      // Do not report about type_decl as these are usually built-in
      // types.
      if (dynamic_pointer_cast<type_decl>(i->second))
	continue;
      out << indent
	  << "  '"
	  << i->second->get_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }

  if (emitted)
    out << "\n";

  emitted = false;
  for (string_decl_base_sptr_map::const_iterator i =
	 d.priv_->inserted_decls_.begin();
       i != d.priv_->inserted_decls_.end();
       ++i)
    {
      // Do not report about type_decl as these are usually built-in
      // types.
      if (dynamic_pointer_cast<type_decl>(i->second))
	continue;
      out << indent
	  << "  '"
	  << i->second->get_pretty_representation()
	  << "' was added\n";
      emitted = true;
    }

  if (emitted)
    out << "\n";
}

/// Report the changes carried by a @ref class_or_union_diff node in a
/// textual format.
///
/// @param d the @ref class_or_union_diff node to consider.
///
/// @param out the output stream to write the textual report to.
///
/// @param indent the number of white space to use as indentation.
void
default_reporter::report(const class_or_union_diff& d,
			 ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  class_or_union_sptr first = d.first_class_or_union(),
    second = d.second_class_or_union();

  const diff_context_sptr& ctxt = d.context();

  // Report class decl-only -> definition change.
  if (ctxt->get_allowed_category() & CLASS_DECL_ONLY_DEF_CHANGE_CATEGORY)
    if (filtering::has_class_decl_only_def_change(first, second))
      {
	string was =
	  first->get_is_declaration_only()
	  ? " was a declaration-only type"
	  : " was a defined type";

	string is_now =
	  second->get_is_declaration_only()
	  ? " and is now a declaration-only type"
	  : " and is now a defined type";

	out << indent << "type " << first->get_pretty_representation()
	    << was << is_now;
	return;
      }

  // member functions
  if (d.member_fns_changes())
    {
      // report deletions
      int numdels = d.get_priv()->deleted_member_functions_.size();
      size_t num_filtered =
	d.get_priv()->count_filtered_deleted_mem_fns(ctxt);
      if (numdels)
	report_mem_header(out, numdels, num_filtered, del_kind,
			  "member function", indent);
      bool emitted = false;
      for (string_member_function_sptr_map::const_iterator i =
	     d.get_priv()->deleted_member_functions_.begin();
	   i != d.get_priv()->deleted_member_functions_.end();
	   ++i)
	{
	  if (!(ctxt->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !get_member_function_is_virtual(i->second))
	    continue;

	  if (emitted
	      && i != d.get_priv()->deleted_member_functions_.begin())
	    out << "\n";
	  method_decl_sptr mem_fun = i->second;
	  out << indent << "  ";
	  represent(*ctxt, mem_fun, out);
	  emitted = true;
	}
      if (emitted)
	out << "\n";

      // report insertions;
      int numins = d.get_priv()->inserted_member_functions_.size();
      num_filtered = d.get_priv()->count_filtered_inserted_mem_fns(ctxt);
      if (numins)
	report_mem_header(out, numins, num_filtered, ins_kind,
			  "member function", indent);
      emitted = false;
      for (string_member_function_sptr_map::const_iterator i =
	     d.get_priv()->inserted_member_functions_.begin();
	   i != d.get_priv()->inserted_member_functions_.end();
	   ++i)
	{
	  if (!(ctxt->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !get_member_function_is_virtual(i->second))
	    continue;

	  if (emitted
	      && i != d.get_priv()->inserted_member_functions_.begin())
	    out << "\n";
	  method_decl_sptr mem_fun = i->second;
	  out << indent << "  ";
	  represent(*ctxt, mem_fun, out);
	  emitted = true;
	}
      if (emitted)
	out << "\n";

      // report member function with sub-types changes
      int numchanges = d.get_priv()->sorted_changed_member_functions_.size();
      num_filtered = d.get_priv()->count_filtered_changed_mem_fns(ctxt);
      if (numchanges)
	report_mem_header(out, numchanges, num_filtered, change_kind,
			  "member function", indent);
      emitted = false;
      for (function_decl_diff_sptrs_type::const_iterator i =
	     d.get_priv()->sorted_changed_member_functions_.begin();
	   i != d.get_priv()->sorted_changed_member_functions_.end();
	   ++i)
	{
	  if (!(ctxt->get_allowed_category()
		& NON_VIRT_MEM_FUN_CHANGE_CATEGORY)
	      && !(get_member_function_is_virtual
		   ((*i)->first_function_decl()))
	      && !(get_member_function_is_virtual
		   ((*i)->second_function_decl())))
	    continue;

	  diff_sptr diff = *i;
	  if (!diff || !diff->to_be_reported())
	    continue;

	  string repr =
	    (*i)->first_function_decl()->get_pretty_representation();
	  if (emitted
	      && i != d.get_priv()->sorted_changed_member_functions_.begin())
	    out << "\n";
	  out << indent << "  '" << repr << "' has some sub-type changes:\n";
	  diff->report(out, indent + "    ");
	  emitted = true;
	}
      if (numchanges)
	out << "\n";
    }

  // data members
  if (d.data_members_changes())
    {
      // report deletions
      int numdels = d.class_or_union_diff::get_priv()->
	get_deleted_non_static_data_members_number();
      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "data member", indent);
	  vector<decl_base_sptr> sorted_dms;
	  sort_data_members
	    (d.class_or_union_diff::get_priv()->deleted_data_members_,
	     sorted_dms);
	  bool emitted = false;
	  for (vector<decl_base_sptr>::const_iterator i = sorted_dms.begin();
	       i != sorted_dms.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(*i);
	      assert(data_mem);
	      if (get_member_is_static(data_mem))
		continue;
	      if (emitted)
		out << "\n";
	      out << indent << "  ";
	      represent_data_member(data_mem, ctxt, out);
	      emitted = true;
	    }
	  if (emitted)
	    out << "\n";
	}

      //report insertions
      int numins =
	d.class_or_union_diff::get_priv()->inserted_data_members_.size();
      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "data member", indent);
	  vector<decl_base_sptr> sorted_dms;
	  sort_data_members
	    (d.class_or_union_diff::get_priv()->inserted_data_members_,
	     sorted_dms);
	  for (vector<decl_base_sptr>::const_iterator i = sorted_dms.begin();
	       i != sorted_dms.end();
	       ++i)
	    {
	      var_decl_sptr data_mem =
		dynamic_pointer_cast<var_decl>(*i);
	      assert(data_mem);
	      out << indent << "  ";
	      represent_data_member(data_mem, ctxt, out);
	    }
	}

      // report change
      size_t numchanges =
	d.class_or_union_diff::get_priv()->sorted_subtype_changed_dm_.size();
      size_t num_filtered =
	d.class_or_union_diff::get_priv()->count_filtered_subtype_changed_dm();
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered,
			    subtype_change_kind, "data member", indent);
	  for (var_diff_sptrs_type::const_iterator it =
		 d.class_or_union_diff::get_priv()->sorted_subtype_changed_dm_.begin();
	       it != d.class_or_union_diff::get_priv()->sorted_subtype_changed_dm_.end();
	       ++it)
	    {
	      if ((*it)->to_be_reported())
		{
		  represent(*it, ctxt, out, indent + " ");
		  out << "\n";
		}
	    }
	}

      numchanges = d.class_or_union_diff::get_priv()->sorted_changed_dm_.size();
      num_filtered =
	d.class_or_union_diff::get_priv()->count_filtered_changed_dm();
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered,
			    change_kind, "data member", indent);
	  for (var_diff_sptrs_type::const_iterator it =
		 d.class_or_union_diff::get_priv()->sorted_changed_dm_.begin();
	       it != d.class_or_union_diff::get_priv()->sorted_changed_dm_.end();
	       ++it)
	    {
	      if ((*it)->to_be_reported())
		{
		  represent(*it, ctxt, out, indent + " ");
		  out << "\n";
		}
	    }
	}
    }

  // member types
  if (const edit_script& e = d.member_types_changes())
    {
      int numchanges =
	d.class_or_union_diff::get_priv()->sorted_changed_member_types_.size();
      int numdels =
	d.class_or_union_diff::get_priv()->deleted_member_types_.size();

      // report deletions
      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "member type", indent);

	  for (string_decl_base_sptr_map::const_iterator i =
		 d.class_or_union_diff::get_priv()->deleted_member_types_.begin();
	       i != d.class_or_union_diff::get_priv()->deleted_member_types_.end();
	       ++i)
	    {
	      if (i != d.class_or_union_diff::get_priv()->deleted_member_types_.begin())
		out << "\n";
	      decl_base_sptr mem_type = i->second;
	      out << indent << "  '"
		  << mem_type->get_pretty_representation()
		  << "'";
	    }
	  out << "\n\n";
	}
      // report changes
      if (numchanges)
	{
	  report_mem_header(out, numchanges, 0, change_kind,
			    "member type", indent);

	  for (diff_sptrs_type::const_iterator it =
		 d.class_or_union_diff::get_priv()->sorted_changed_member_types_.begin();
	       it != d.class_or_union_diff::get_priv()->sorted_changed_member_types_.end();
	       ++it)
	    {
	      if (!(*it)->to_be_reported())
		continue;

	      type_or_decl_base_sptr o = (*it)->first_subject();
	      type_or_decl_base_sptr n = (*it)->second_subject();
	      out << indent << "  '"
		  << o->get_pretty_representation()
		  << "' changed ";
	      report_loc_info(n, *ctxt, out);
	      out << ":\n";
	      (*it)->report(out, indent + "    ");
	    }
	  out << "\n";
	}

      // report insertions
      int numins = e.num_insertions();
      assert(numchanges <= numins);
      numins -= numchanges;

      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "member type", indent);

	  bool emitted = false;
	  for (vector<insertion>::const_iterator i = e.insertions().begin();
	       i != e.insertions().end();
	       ++i)
	    {
	      type_base_sptr mem_type;
	      for (vector<unsigned>::const_iterator j =
		     i->inserted_indexes().begin();
		   j != i->inserted_indexes().end();
		   ++j)
		{
		  if (emitted)
		    out << "\n";
		  mem_type = second->get_member_types()[*j];
		  if (!d.class_or_union_diff::get_priv()->
		      member_type_has_changed(get_type_declaration(mem_type)))
		    {
		      out << indent << "  '"
			  << get_type_declaration(mem_type)->
			get_pretty_representation()
			  << "'";
		      emitted = true;
		    }
		}
	    }
	  out << "\n\n";
	}
    }

  // member function templates
  if (const edit_script& e = d.member_fn_tmpls_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, 0, del_kind,
			  "member function template", indent);
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  if (i != e.deletions().begin())
	    out << "\n";
	  member_function_template_sptr mem_fn_tmpl =
	    first->get_member_function_templates()[i->index()];
	  out << indent << "  '"
	      << mem_fn_tmpl->as_function_tdecl()->get_pretty_representation()
	      << "'";
	}
      if (numdels)
	out << "\n\n";

      // report insertions
      int numins = e.num_insertions();
      if (numins)
	report_mem_header(out, numins, 0, ins_kind,
			  "member function template", indent);
      bool emitted = false;
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  member_function_template_sptr mem_fn_tmpl;
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      if (emitted)
		out << "\n";
	      mem_fn_tmpl = second->get_member_function_templates()[*j];
	      out << indent << "  '"
		  << mem_fn_tmpl->as_function_tdecl()->
		get_pretty_representation()
		  << "'";
	      emitted = true;
	    }
	}
      if (numins)
	out << "\n\n";
    }

  // member class templates.
  if (const edit_script& e = d.member_class_tmpls_changes())
    {
      // report deletions
      int numdels = e.num_deletions();
      if (numdels)
	report_mem_header(out, numdels, 0, del_kind,
			  "member class template", indent);
      for (vector<deletion>::const_iterator i = e.deletions().begin();
	   i != e.deletions().end();
	   ++i)
	{
	  if (i != e.deletions().begin())
	    out << "\n";
	  member_class_template_sptr mem_cls_tmpl =
	    first->get_member_class_templates()[i->index()];
	  out << indent << "  '"
	      << mem_cls_tmpl->as_class_tdecl()->get_pretty_representation()
	      << "'";
	}
      if (numdels)
	out << "\n\n";

      // report insertions
      int numins = e.num_insertions();
      if (numins)
	report_mem_header(out, numins, 0, ins_kind,
			  "member class template", indent);
      bool emitted = false;
      for (vector<insertion>::const_iterator i = e.insertions().begin();
	   i != e.insertions().end();
	   ++i)
	{
	  member_class_template_sptr mem_cls_tmpl;
	  for (vector<unsigned>::const_iterator j =
		 i->inserted_indexes().begin();
	       j != i->inserted_indexes().end();
	       ++j)
	    {
	      if (emitted)
		out << "\n";
	      mem_cls_tmpl = second->get_member_class_templates()[*j];
	      out << indent << "  '"
		  << mem_cls_tmpl->as_class_tdecl()
		->get_pretty_representation()
		  << "'";
	      emitted = true;
	    }
	}
      if (numins)
	out << "\n\n";
    }
}

/// Produce a basic report about the changes carried by a @ref
/// class_diff node.
///
/// @param d the @ref class_diff node to consider.
///
/// @param out the output stream to report the changes to.
///
/// @param indent the string to use as an indentation prefix in the
/// report.
void
default_reporter::report(const class_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  string name = d.first_subject()->get_pretty_representation();

  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(d.first_subject(),
						   d.second_subject());

  d.currently_reporting(true);

  // Now report the changes about the differents parts of the type.
  class_decl_sptr first = d.first_class_decl(),
    second = d.second_class_decl();

  if (report_name_size_and_alignment_changes(first, second, d.context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  const diff_context_sptr& ctxt = d.context();
  maybe_report_diff_for_member(first, second, ctxt, out, indent);

  // bases classes
  if (d.base_changes())
    {
      // Report deletions.
      int numdels = d.get_priv()->deleted_bases_.size();
      size_t numchanges = d.get_priv()->sorted_changed_bases_.size();

      if (numdels)
	{
	  report_mem_header(out, numdels, 0, del_kind,
			    "base class", indent);

	  for (class_decl::base_specs::const_iterator i
		 = d.get_priv()->sorted_deleted_bases_.begin();
	       i != d.get_priv()->sorted_deleted_bases_.end();
	       ++i)
	    {
	      if (i != d.get_priv()->sorted_deleted_bases_.begin())
		out << "\n";

	      class_decl::base_spec_sptr base = *i;

	      if (d.get_priv()->base_has_changed(base))
		continue;
	      out << indent << "  "
		  << base->get_base_class()->get_pretty_representation();
	      report_loc_info(base->get_base_class(), *d.context(), out);
	    }
	  out << "\n";
	}

      // Report changes.
      bool emitted = false;
      size_t num_filtered = d.get_priv()->count_filtered_bases();
      if (numchanges)
	{
	  report_mem_header(out, numchanges, num_filtered, change_kind,
			    "base class", indent);
	  for (base_diff_sptrs_type::const_iterator it =
		 d.get_priv()->sorted_changed_bases_.begin();
	       it != d.get_priv()->sorted_changed_bases_.end();
	       ++it)
	    {
	      base_diff_sptr diff = *it;
	      if (!diff || !diff->to_be_reported())
		continue;

	      class_decl::base_spec_sptr o = diff->first_base();
	      out << indent << "  '"
		  << o->get_base_class()->get_pretty_representation() << "'";
	      report_loc_info(o->get_base_class(), *d.context(), out);
	      out << " changed:\n";
	      diff->report(out, indent + "    ");
	      emitted = true;
	    }
	  if (emitted)
	    out << "\n";
	}

      //Report insertions.
      int numins = d.get_priv()->inserted_bases_.size();
      if (numins)
	{
	  report_mem_header(out, numins, 0, ins_kind,
			    "base class", indent);

	  bool emitted = false;
	  for (class_decl::base_specs::const_iterator i =
		 d.get_priv()->sorted_inserted_bases_.begin();
	       i != d.get_priv()->sorted_inserted_bases_.end();
	       ++i)
	    {
	      class_decl_sptr b = (*i)->get_base_class();
	      if (emitted)
		out << "\n";
	      out << indent << "  " << b->get_pretty_representation();
	      report_loc_info(b, *ctxt, out);
	      emitted = true;
	    }
	  out << "\n";
	}
    }

  d.class_or_union_diff::report(out, indent);

  d.currently_reporting(false);

  d.reported_once(true);
}

/// Produce a basic report about the changes carried by a @ref
/// union_diff node.
///
/// @param d the @ref union_diff node to consider.
///
/// @param out the output stream to report the changes to.
///
/// @param indent the string to use as an indentation prefix in the
/// report.
void
default_reporter::report(const union_diff& d, ostream& out,
			 const string& indent) const
{
  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER(d.first_subject(),
						   d.second_subject());

  d.currently_reporting(true);

  // Now report the changes about the differents parts of the type.
  union_decl_sptr first = d.first_union_decl(), second = d.second_union_decl();

  if (report_name_size_and_alignment_changes(first, second, d.context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  maybe_report_diff_for_member(first, second,d. context(), out, indent);

  d.class_or_union_diff::report(out, indent);

 d.currently_reporting(false);

  d.reported_once(true);
}

/// Emit a report about the changes carried by a @ref distinct_diff
/// node.
///
/// @param d the @ref distinct_diff node to consider.
///
/// @param out the output stream to send the diff report to.
///
/// @param indent the indentation string to use in the report.
void
default_reporter::report(const distinct_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  type_or_decl_base_sptr f = d.first(), s = d.second();

  string f_repr = f ? f->get_pretty_representation() : "'void'";
  string s_repr = s ? s->get_pretty_representation() : "'void'";

  diff_sptr diff = d.compatible_child_diff();

  string compatible = diff ? " to compatible type '": " to '";

  out << indent << "entity changed from '" << f_repr << "'"
      << compatible << s_repr << "'";
  report_loc_info(s, *d.context(), out);
  out << "\n";

  type_base_sptr fs = strip_typedef(is_type(f)),
    ss = strip_typedef(is_type(s));

  if (diff)
    diff->report(out, indent + "  ");
  else
    if (report_size_and_alignment_changes(f, s, d.context(), out, indent,
					  /*start_with_new_line=*/false))
      out << "\n";
}

/// Serialize a report of the changes encapsulated in the current
/// instance of @ref function_decl_diff over to an output stream.
///
/// @param d the @ref function_decl_diff node to consider.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the string to use an an indentation prefix.
void
default_reporter::report(const function_decl_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  maybe_report_diff_for_member(d.first_function_decl(),
			       d.second_function_decl(),
			       d.context(), out, indent);

  function_decl_sptr ff = d.first_function_decl();
  function_decl_sptr sf = d.second_function_decl();

  diff_context_sptr ctxt = d.context();
  corpus_sptr fc = ctxt->get_first_corpus();
  corpus_sptr sc = ctxt->get_second_corpus();


  string qn1 = ff->get_qualified_name(), qn2 = sf->get_qualified_name(),
    linkage_names1, linkage_names2;
  elf_symbol_sptr s1 = ff->get_symbol(), s2 = sf->get_symbol();

  if (s1)
    linkage_names1 = s1->get_id_string();
  if (s2)
    linkage_names2 = s2->get_id_string();

  // If the symbols for ff and sf have aliases, get all the names of
  // the aliases;
  if (fc && s1)
    linkage_names1 =
      s1->get_aliases_id_string(fc->get_fun_symbol_map());
  if (sc && s2)
    linkage_names2 =
      s2->get_aliases_id_string(sc->get_fun_symbol_map());

  /// If the set of linkage names of the function have changed, report
  /// it.
  if (linkage_names1 != linkage_names2)
    {
      if (linkage_names1.empty())
	{
	  out << indent << ff->get_pretty_representation()
	      << " didn't have any linkage name, and it now has: '"
	      << linkage_names2 << "'\n";
	}
      else if (linkage_names2.empty())
	{
	  out << indent << ff->get_pretty_representation()
	      << " did have linkage names '" << linkage_names1
	      << "'\n"
	      << indent << "but it doesn't have any linkage name anymore\n";
	}
      else
	out << indent << "linkage names of "
	    << ff->get_pretty_representation()
	    << "\n" << indent << "changed from '"
	    << linkage_names1 << "' to '" << linkage_names2 << "'\n";
    }

  if (qn1 != qn2
      && d.type_diff()
      && d.type_diff()->to_be_reported())
    {
      // So the function has sub-type changes that are to be
      // reported.  Let's see if the function name changed too; if it
      // did, then we'd report that change right before reporting the
      // sub-type changes.
      string frep1 = d.first_function_decl()->get_pretty_representation(),
	frep2 = d.second_function_decl()->get_pretty_representation();
      out << indent << "'" << frep1 << " {" << linkage_names1<< "}"
	  << "' now becomes '"
	  << frep2 << " {" << linkage_names2 << "}" << "'\n";
    }

  maybe_report_diff_for_symbol(ff->get_symbol(),
			       sf->get_symbol(),
			       out, indent);

  // Now report about inline-ness changes
  if (ff->is_declared_inline() != sf->is_declared_inline())
    {
      out << indent;
      if (ff->is_declared_inline())
	out << sf->get_pretty_representation()
	    << " is not declared inline anymore\n";
      else
	out << sf->get_pretty_representation()
	    << " is now declared inline\n";
    }

  // Report about vtable offset changes.
  if (is_member_function(ff) && is_member_function(sf))
    {
      bool ff_is_virtual = get_member_function_is_virtual(ff),
	sf_is_virtual = get_member_function_is_virtual(sf);
      if (ff_is_virtual != sf_is_virtual)
	{
	  out << indent;
	  if (ff_is_virtual)
	    out << ff->get_pretty_representation()
		<< " is no more declared virtual\n";
	  else
	    out << ff->get_pretty_representation()
		<< " is now declared virtual\n";
	}

      size_t ff_vtable_offset = get_member_function_vtable_offset(ff),
	sf_vtable_offset = get_member_function_vtable_offset(sf);
      if (ff_is_virtual && sf_is_virtual
	  && (ff_vtable_offset != sf_vtable_offset))
	{
	  out << indent
	      << "the vtable offset of "  << ff->get_pretty_representation()
	      << " changed from " << ff_vtable_offset
	      << " to " << sf_vtable_offset << "\n";
	}

      // the classes of the two member functions.
      class_decl_sptr fc =
	is_class_type(is_method_type(ff->get_type())->get_class_type());
      class_decl_sptr sc =
	is_class_type(is_method_type(sf->get_type())->get_class_type());

      // Detect if the virtual member function changes above
      // introduced a vtable change or not.
      bool vtable_added = false, vtable_removed = false;
      if (!fc->get_is_declaration_only() && !sc->get_is_declaration_only())
	{
	  vtable_added = !fc->has_vtable() && sc->has_vtable();
	  vtable_removed = fc->has_vtable() && !sc->has_vtable();
	}
      bool vtable_changed = ((ff_is_virtual != sf_is_virtual)
			     || (ff_vtable_offset != sf_vtable_offset));
      bool incompatible_change = (ff_vtable_offset != sf_vtable_offset);

      if (vtable_added)
	out << indent
	    << "  note that a vtable was added to "
	    << fc->get_pretty_representation()
	    << "\n";
      else if (vtable_removed)
	out << indent
	    << "  note that the vtable was removed from "
	    << fc->get_pretty_representation()
	    << "\n";
      else if (vtable_changed)
	{
	  out << indent;
	  if (incompatible_change)
	    out << "  note that this is an ABI incompatible "
	      "change to the vtable of ";
	  else
	    out << "  note that this induces a change to the vtable of ";
	  out << fc->get_pretty_representation()
	      << "\n";
	}

    }

  // Report about function type differences.
  if (d.type_diff() && d.type_diff()->to_be_reported())
    d.type_diff()->report(out, indent);
}

/// Report the changes carried by a @ref var_diff node in a serialized
/// form.
///
/// @param d the @ref var_diff node to consider.
///
/// @param out the stream to serialize the diff to.
///
/// @param indent the prefix to use for the indentation of this
/// serialization.
void
default_reporter::report(const var_diff& d, ostream& out,
			 const string& indent) const
{
  if (!d.to_be_reported())
    return;

  decl_base_sptr first = d.first_var(), second = d.second_var();
  string n = first->get_pretty_representation();

  if (report_name_size_and_alignment_changes(first, second,
					     d.context(),
					     out, indent,
					     /*start_with_new_line=*/false))
    out << "\n";

  maybe_report_diff_for_symbol(d.first_var()->get_symbol(),
			       d.second_var()->get_symbol(),
			       out, indent);

  maybe_report_diff_for_member(first, second, d.context(), out, indent);

  if (diff_sptr dif = d.type_diff())
    {
      if (dif->to_be_reported())
	{
	  RETURN_IF_BEING_REPORTED_OR_WAS_REPORTED_EARLIER2(dif, "type");
	  out << indent << "type of variable changed:\n";
	  dif->report(out, indent + " ");
	}
    }
}

/// Report the changes carried by a @ref translation_unit_diff node in
/// a serialized form.
///
/// @param d the @ref translation_unit_diff node to consider.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the prefix to use as indentation for the report.
void
default_reporter::report(const translation_unit_diff& d,
			 ostream& out,
			 const string& indent) const
{
  static_cast<const scope_diff&>(d).report(out, indent);
}

/// Report the changes carried by a @ref corpus_diff node in a
/// serialized form.
///
/// @param d the @ref corpus_diff node to consider.
///
/// @param out the output stream to serialize the report to.
///
/// @param indent the prefix to use as indentation for the report.
void
default_reporter::report(const corpus_diff& d, ostream& out,
			 const string& indent) const
{
  size_t total = 0, removed = 0, added = 0;
  const corpus_diff::diff_stats &s =
    const_cast<corpus_diff&>(d).
    apply_filters_and_suppressions_before_reporting();

  const diff_context_sptr& ctxt = d.context();

  /// Report removed/added/changed functions.
  total = s.net_num_func_removed() + s.net_num_func_added() +
    s.net_num_func_changed();
  const unsigned large_num = 100;

  d.priv_->emit_diff_stats(s, out, indent);
  if (ctxt->show_stats_only())
    return;
  out << "\n";

  if (ctxt->show_soname_change()
      && !d.priv_->sonames_equal_)
    out << indent << "SONAME changed from '"
	<< d.first_corpus()->get_soname() << "' to '"
	<< d.second_corpus()->get_soname() << "'\n\n";

  if (ctxt->show_architecture_change()
      && !d.priv_->architectures_equal_)
    out << indent << "architecture changed from '"
	<< d.first_corpus()->get_architecture_name() << "' to '"
	<< d.second_corpus()->get_architecture_name() << "'\n\n";

  if (ctxt->show_deleted_fns())
    {
      if (s.net_num_func_removed() == 1)
	out << indent << "1 Removed function:\n\n";
      else if (s.net_num_func_removed() > 1)
	out << indent << s.net_num_func_removed() << " Removed functions:\n\n";

      vector<function_decl*>sorted_deleted_fns;
      sort_string_function_ptr_map(d.priv_->deleted_fns_, sorted_deleted_fns);
      for (vector<function_decl*>::const_iterator i =
	     sorted_deleted_fns.begin();
	   i != sorted_deleted_fns.end();
	   ++i)
	{
	  if (d.priv_->deleted_function_is_suppressed(*i))
	    continue;

	  out << indent
	      << "  ";
	  if (total > large_num)
	    out << "[D] ";
	  out << "'" << (*i)->get_pretty_representation() << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *(*i)->get_symbol(),
					    d.first_corpus()->get_fun_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  if (is_member_function(*i) && get_member_function_is_virtual(*i))
	    {
	      class_decl_sptr c =
		is_class_type(is_method_type((*i)->get_type())->get_class_type());
	      out << indent
		  << "    "
		  << "note that this removes an entry from the vtable of "
		  << c->get_pretty_representation()
		  << "\n";
	    }
	  ++removed;
	}
      if (removed)
	out << "\n";
    }

  if (ctxt->show_added_fns())
    {
      if (s.net_num_func_added() == 1)
	out << indent << "1 Added function:\n\n";
      else if (s.net_num_func_added() > 1)
	out << indent << s.net_num_func_added()
	    << " Added functions:\n\n";
      vector<function_decl*> sorted_added_fns;
      sort_string_function_ptr_map(d.priv_->added_fns_, sorted_added_fns);
      for (vector<function_decl*>::const_iterator i = sorted_added_fns.begin();
	   i != sorted_added_fns.end();
	   ++i)
	{
	  if (d.priv_->added_function_is_suppressed(*i))
	    continue;

	  out
	    << indent
	    << "  ";
	  if (total > large_num)
	    out << "[A] ";
	  out << "'"
	      << (*i)->get_pretty_representation()
	      << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases
		(out, "", *(*i)->get_symbol(),
		 d.second_corpus()->get_fun_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  if (is_member_function(*i) && get_member_function_is_virtual(*i))
	    {
	      class_decl_sptr c =
		is_class_type(is_method_type((*i)->get_type())->get_class_type());
	      out << indent
		  << "    "
		  << "note that this adds a new entry to the vtable of "
		  << c->get_pretty_representation()
		  << "\n";
	    }
	  ++added;
	}
      if (added)
	{
	  out << "\n";
	  added = false;
	}
    }

  if (ctxt->show_changed_fns())
    {
      size_t num_changed = s.num_func_changed() - s.num_changed_func_filtered_out();
      if (num_changed == 1)
	out << indent << "1 function with some indirect sub-type change:\n\n";
      else if (num_changed > 1)
	out << indent << num_changed
	    << " functions with some indirect sub-type change:\n\n";

      bool emitted = false;
      vector<function_decl_diff_sptr> sorted_changed_fns;
      sort_string_function_decl_diff_sptr_map(d.priv_->changed_fns_map_,
					      sorted_changed_fns);
      for (vector<function_decl_diff_sptr>::const_iterator i =
	     sorted_changed_fns.begin();
	   i != sorted_changed_fns.end();
	   ++i)
	{
	  diff_sptr diff = *i;
	  if (!diff)
	    continue;

	  if (diff->to_be_reported())
	    {
	      function_decl_sptr fn = (*i)->first_function_decl();
	      out << indent << "  [C]'"
		  << fn->get_pretty_representation() << "'";
	      report_loc_info((*i)->second_function_decl(), *ctxt, out);
	      out << " has some indirect sub-type changes:\n";
	      if ((fn->get_symbol()->has_aliases()
		   && !(is_member_function(fn)
			&& get_member_function_is_ctor(fn))
		   && !(is_member_function(fn)
			&& get_member_function_is_dtor(fn)))
		  || (is_c_language(get_translation_unit(fn)->get_language())
		      && fn->get_name() != fn->get_linkage_name()))
		{
		  int number_of_aliases =
		    fn->get_symbol()->get_number_of_aliases();
		  if (number_of_aliases == 0)
		    {
		      out << indent << "    "
			  << "Please note that the exported symbol of "
			"this function is "
			  << fn->get_symbol()->get_id_string()
			  << "\n";
		    }
		  else
		    {
		      out << indent << "    "
			  << "Please note that the symbol of this function is "
			  << fn->get_symbol()->get_id_string()
			  << "\n     and it aliases symbol";
		      if (number_of_aliases > 1)
			out << "s";
		      out << ": "
			  << fn->get_symbol()->get_aliases_id_string(false)
			  << "\n";
		    }
		}
	      diff->report(out, indent + "    ");
	      out << "\n";
	      emitted |= true;
	    }
	}
      if (emitted)
	{
	  out << "\n";
	  emitted = false;
	}
    }

  // Report added/removed/changed variables.
  total = s.num_vars_removed() + s.num_vars_added() +
    s.num_vars_changed() - s.num_changed_vars_filtered_out();

  if (ctxt->show_deleted_vars())
    {
      if (s.net_num_vars_removed() == 1)
	out << indent << "1 Removed variable:\n\n";
      else if (s.net_num_vars_removed() > 1)
	out << indent << s.net_num_vars_removed()
	    << " Removed variables:\n\n";
      string n;
      vector<var_decl*> sorted_deleted_vars;
      sort_string_var_ptr_map(d.priv_->deleted_vars_, sorted_deleted_vars);
      for (vector<var_decl*>::const_iterator i =
	     sorted_deleted_vars.begin();
	   i != sorted_deleted_vars.end();
	   ++i)
	{
	  if (d.priv_->deleted_variable_is_suppressed(*i))
	    continue;

	  n = (*i)->get_pretty_representation();

	  out << indent
	      << "  ";
	  if (total > large_num)
	    out << "[D] ";
	  out << "'"
	      << n
	      << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *(*i)->get_symbol(),
					    d.first_corpus()->get_var_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  ++removed;
	}
      if (removed)
	{
	  out << "\n";
	  removed = 0;
	}
    }

  if (ctxt->show_added_vars())
    {
      if (s.net_num_vars_added() == 1)
	out << indent << "1 Added variable:\n\n";
      else if (s.net_num_vars_added() > 1)
	out << indent << s.net_num_vars_added()
	    << " Added variables:\n\n";
      string n;
      vector<var_decl*> sorted_added_vars;
      sort_string_var_ptr_map(d.priv_->added_vars_, sorted_added_vars);
      for (vector<var_decl*>::const_iterator i =
	     sorted_added_vars.begin();
	   i != sorted_added_vars.end();
	   ++i)
	{
	  if (d.priv_->added_variable_is_suppressed(*i))
	    continue;

	  n = (*i)->get_pretty_representation();

	  out << indent
	      << "  ";
	  if (total > large_num)
	    out << "[A] ";
	  out << "'" << n << "'";
	  if (ctxt->show_linkage_names())
	    {
	      out << "    {";
	      show_linkage_name_and_aliases(out, "", *(*i)->get_symbol(),
					    d.second_corpus()->get_var_symbol_map());
	      out << "}";
	    }
	  out << "\n";
	  ++added;
	}
      if (added)
	{
	  out << "\n";
	  added = 0;
	}
    }

  if (ctxt->show_changed_vars())
    {
      size_t num_changed = s.num_vars_changed() - s.num_changed_vars_filtered_out();
      if (num_changed == 1)
	out << indent << "1 Changed variable:\n\n";
      else if (num_changed > 1)
	out << indent << num_changed
	    << " Changed variables:\n\n";
      string n1, n2;

      for (var_diff_sptrs_type::const_iterator i =
	     d.priv_->sorted_changed_vars_.begin();
	   i != d.priv_->sorted_changed_vars_.end();
	   ++i)
	{
	  diff_sptr diff = *i;

	  if (!diff)
	    continue;

	  if (!diff->to_be_reported())
	    continue;

	  n1 = diff->first_subject()->get_pretty_representation();
	  n2 = diff->second_subject()->get_pretty_representation();

	  out << indent << "  [C]'" << n1 << "' was changed";
	  if (n1 != n2)
	    out << " to '" << n2 << "'";
	  report_loc_info(diff->second_subject(), *ctxt, out);
	  out << ":\n";
	  diff->report(out, indent + "    ");
	  out << "\n";
	}
      if (num_changed)
	out << "\n";
    }

  // Report removed function symbols not referenced by any debug info.
  if (ctxt->show_symbols_unreferenced_by_debug_info()
      && d.priv_->deleted_unrefed_fn_syms_.size())
    {
      if (s.net_num_removed_func_syms() == 1)
	out << indent
	    << "1 Removed function symbol not referenced by debug info:\n\n";
      else if (s.net_num_removed_func_syms() > 0)
	out << indent
	    << s.net_num_removed_func_syms()
	    << " Removed function symbols not referenced by debug info:\n\n";

      vector<elf_symbol_sptr> sorted_deleted_unrefed_fn_syms;
      sort_string_elf_symbol_map(d.priv_->deleted_unrefed_fn_syms_,
				 sorted_deleted_unrefed_fn_syms);
      for (vector<elf_symbol_sptr>::const_iterator i =
	     sorted_deleted_unrefed_fn_syms.begin();
	   i != sorted_deleted_unrefed_fn_syms.end();
	   ++i)
	{
	  if (d.priv_->deleted_unrefed_fn_sym_is_suppressed((*i).get()))
	    continue;

	  out << indent << "  ";
	  if (s.net_num_removed_func_syms() > large_num)
	    out << "[D] ";

	  show_linkage_name_and_aliases(out, "", **i,
					d.first_corpus()->get_fun_symbol_map());
	  out << "\n";
	}
      if (sorted_deleted_unrefed_fn_syms.size())
	out << '\n';
    }

  // Report added function symbols not referenced by any debug info.
  if (ctxt->show_symbols_unreferenced_by_debug_info()
      && ctxt->show_added_symbols_unreferenced_by_debug_info()
      && d.priv_->added_unrefed_fn_syms_.size())
    {
      if (s.net_num_added_func_syms() == 1)
	out << indent
	    << "1 Added function symbol not referenced by debug info:\n\n";
      else if (s.net_num_added_func_syms() > 0)
	out << indent
	    << s.net_num_added_func_syms()
	    << " Added function symbols not referenced by debug info:\n\n";

      vector<elf_symbol_sptr> sorted_added_unrefed_fn_syms;
      sort_string_elf_symbol_map(d.priv_->added_unrefed_fn_syms_,
				 sorted_added_unrefed_fn_syms);
      for (vector<elf_symbol_sptr>::const_iterator i =
	     sorted_added_unrefed_fn_syms.begin();
	   i != sorted_added_unrefed_fn_syms.end();
	   ++i)
	{
	  if (d.priv_->added_unrefed_fn_sym_is_suppressed((*i).get()))
	    continue;

	  out << indent << "  ";
	  if (s.net_num_added_func_syms() > large_num)
	    out << "[A] ";
	  show_linkage_name_and_aliases(out, "",
					**i,
					d.second_corpus()->get_fun_symbol_map());
	  out << "\n";
	}
      if (sorted_added_unrefed_fn_syms.size())
	out << '\n';
    }

  // Report removed variable symbols not referenced by any debug info.
  if (ctxt->show_symbols_unreferenced_by_debug_info()
      && d.priv_->deleted_unrefed_var_syms_.size())
    {
      if (s.net_num_removed_var_syms() == 1)
	out << indent
	    << "1 Removed variable symbol not referenced by debug info:\n\n";
      else if (s.net_num_removed_var_syms() > 0)
	out << indent
	    << s.net_num_removed_var_syms()
	    << " Removed variable symbols not referenced by debug info:\n\n";

      vector<elf_symbol_sptr> sorted_deleted_unrefed_var_syms;
      sort_string_elf_symbol_map(d.priv_->deleted_unrefed_var_syms_,
				 sorted_deleted_unrefed_var_syms);
      for (vector<elf_symbol_sptr>::const_iterator i =
	     sorted_deleted_unrefed_var_syms.begin();
	   i != sorted_deleted_unrefed_var_syms.end();
	   ++i)
	{
	  if (d.priv_->deleted_unrefed_var_sym_is_suppressed((*i).get()))
	    continue;

	  out << indent << "  ";
	  if (s.num_var_syms_removed() > large_num)
	    out << "[D] ";

	  show_linkage_name_and_aliases
	    (out, "", **i,
	     d.first_corpus()->get_fun_symbol_map());

	  out << "\n";
	}
      if (sorted_deleted_unrefed_var_syms.size())
	out << '\n';
    }

  // Report added variable symbols not referenced by any debug info.
  if (ctxt->show_symbols_unreferenced_by_debug_info()
      && ctxt->show_added_symbols_unreferenced_by_debug_info()
      && d.priv_->added_unrefed_var_syms_.size())
    {
      if (s.net_num_added_var_syms() == 1)
	out << indent
	    << "1 Added variable symbol not referenced by debug info:\n\n";
      else if (s.net_num_added_var_syms() > 0)
	out << indent
	    << s.net_num_added_var_syms()
	    << " Added variable symbols not referenced by debug info:\n\n";

      vector<elf_symbol_sptr> sorted_added_unrefed_var_syms;
      sort_string_elf_symbol_map(d.priv_->added_unrefed_var_syms_,
				 sorted_added_unrefed_var_syms);
      for (vector<elf_symbol_sptr>::const_iterator i =
	     sorted_added_unrefed_var_syms.begin();
	   i != sorted_added_unrefed_var_syms.end();
	   ++i)
	{
	  if (d.priv_->added_unrefed_var_sym_is_suppressed((*i).get()))
	    continue;

	  out << indent << "  ";
	  if (s.net_num_added_var_syms() > large_num)
	    out << "[A] ";
	  show_linkage_name_and_aliases(out, "", **i,
					d.second_corpus()->get_fun_symbol_map());
	  out << "\n";
	}
      if (sorted_added_unrefed_var_syms.size())
	out << '\n';
    }

  d.priv_->maybe_dump_diff_tree();
}

} // end namespace comparison
}// end namespace libabigail
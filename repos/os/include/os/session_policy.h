/*
 * \brief  Utilities for handling server-side session policies
 * \author Norman Feske
 * \date   2011-09-13
 */

/*
 * Copyright (C) 2011-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__OS__SESSION_POLICY_H_
#define _INCLUDE__OS__SESSION_POLICY_H_

#include <base/session_label.h>
#include <base/log.h>
#include <base/node.h>
#include <session/session.h>

namespace Genode {

	struct Node_label_score;

	template <size_t N>
	auto with_matching_policy(String<N> const &, auto const &, auto const &,
	                          auto const &no_match_fn) -> decltype(no_match_fn());
}


/**
 * Score for matching an node against a label
 *
 * The score is based on the attributes 'label', 'label_prefix', and
 * 'label_suffix'.
 */
struct Genode::Node_label_score
{
	bool  label_present = true;
	bool prefix_present = true;
	bool suffix_present = true;

	bool label_match = false;

	/*
	 * The match values contain the number of matching characters + 1.
	 * If 0, there is a conflict. If 1, an empty string matched.
	 */
	static constexpr size_t CONFLICT = 0;
	size_t prefix_match = CONFLICT;
	size_t suffix_match = CONFLICT;

	Node_label_score() { }

	template <size_t N>
	Node_label_score(auto const &node, String<N> const &label)
	:
		label_present (node.has_attribute("label")),
		prefix_present(node.has_attribute("label_prefix")),
		suffix_present(node.has_attribute("label_suffix"))
	{
		label.with_span([&] (Span const &span) { span.trimmed([&] (Span const &label) {

			auto equals   = [&] (Span const &exact)  { return label.equals(exact); };
			auto prefixed = [&] (Span const &prefix) { return label.starts_with(prefix); };
			auto suffixed = [&] (Span const &suffix) { return label.ends_with(suffix); };

			auto match = [&] (auto const &match_fn, char const *attr)
			{
				return node.attribute_value(attr, String<N>()).with_span([&] (Span const &span) {
					return span.trimmed([&] (Span const &trimmed_value) {
						return match_fn(trimmed_value)
						     ? trimmed_value.num_bytes + 1 : CONFLICT; }); });
			};

			if (label_present)  label_match  = match(equals,   "label") != CONFLICT;
			if (prefix_present) prefix_match = match(prefixed, "label_prefix");
			if (suffix_present) suffix_match = match(suffixed, "label_suffix");
		}); });
	}

	bool conflict() const
	{
		return (label_present  && !label_match)
		    || (prefix_present && !prefix_match)
		    || (suffix_present && !suffix_match);
	}

	/**
	 * Return true if this node's score is higher than 'other'
	 */
	bool stronger(Node_label_score const &other) const
	{
		/* something must match */
		if (!(label_present || prefix_present || suffix_present))
			return false;

		/* if we are in conflict, we have a lower score than any other node */
		if (conflict())
			return false;

		/* there are no conflicts */

		/* we have a higher score than another conflicting node */
		if (other.conflict())
			return true;

		if (label_present && !other.label_present)
			return true;

		if (other.label_present)
			return false;

		/* labels are equally good */

		if (prefix_present && !other.prefix_present)
			return true;

		if (!prefix_present && other.prefix_present)
			return false;

		if (prefix_present && other.prefix_present) {

			if (prefix_match > other.prefix_match)
				return true;

			if (prefix_match < other.prefix_match)
				return false;
		}

		/* prefixes are equally good */

		if (suffix_present && !other.suffix_present)
			return true;

		if (!suffix_present && other.suffix_present)
			return false;

		if (suffix_present && other.suffix_present) {

			if (suffix_match > other.suffix_match)
				return true;

			if (suffix_match < other.suffix_match)
				return false;
		}

		/* nodes are equally good */

		return false;
	}
};


/**
 * Call 'match_fn' with the policy that matches best the given 'label'
 *
 * \param policies     XML node that contains potentially many '<policy>'
 *                     nodes and an optional '<default-policy>' node.
 * \param match_fn     functor called with best matching policy XML node
 *                     argmument
 * \param no_match_fn  functor called if no matching policy exists
 */
template <Genode::size_t N>
auto Genode::with_matching_policy(String<N> const &label,
                                  auto      const &policies,
                                  auto      const &match_fn,
                                  auto      const &no_match_fn) -> decltype(no_match_fn())
{
	static unsigned const NO_MATCH = ~0U;

	struct Best { unsigned index; Node_label_score score; };

	Best best { NO_MATCH, { } };

	unsigned i = 0;
	policies.for_each_sub_node([&] (auto const &policy) {
		if (policy.has_type("policy")) {
			Node_label_score const score(policy, label);
			if (score.stronger(best.score))
				best = { i, score };
		}
		i++;
	});

	/* fall back to default policy if no match exists */
	if (best.index == NO_MATCH)
		return policies.with_sub_node("default-policy",
			[&] (auto const &policy) { return match_fn(policy); },
			[&]                      { return no_match_fn(); });

	return policies.with_sub_node(best.index,
		[&] (auto const &policy) { return match_fn(policy); },
		[&]                      { return no_match_fn(); });
}

#endif /* _INCLUDE__OS__SESSION_POLICY_H_ */

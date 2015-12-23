//
//  parse.h
//  parse
//
//  Created by Jim Park on 12/12/15.
//  Copyright © 2015 Jim Park. All rights reserved.
//

#ifndef URLPARSER_PARSE_H
#define URLPARSER_PARSE_H

#include <unordered_set>
#include <functional>
#include <string>
#include <iostream>
#include <vector>

namespace parse {

    template<typename C>
    class Node {
    public:
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using charT = C;
        using mfT = std::function<void(strT)>;

        Node() : mf_([](strT) { }) { }

        virtual ~Node() { }

        virtual bool operator()(
                iterT &begin, iterT end) = 0;

        virtual void setMatchCallback(mfT f) { mf_ = std::move(f); }

        virtual void onMatch(iterT begin, iterT end) const {
            mf_(strT{begin, end});
        }

        virtual void reset() {
            mf_(strT{});
        }

    protected:
        mfT mf_;
    };

    template<typename C>
    class End : public Node<C> {
    public:
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using matchT = std::function<bool(C)>;
        using charT = C;

        virtual ~End() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {
            if (begin == end) {
                Node<C>::onMatch(begin, end);
                return true;
            }
            return false;
        }
    };

    template<typename C>
    class LitC : public Node<C> {
    public:
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using matchT = std::function<bool(C)>;
        using charT = C;

        LitC(matchT m)
                : m_(std::move(m)) { }

        virtual ~LitC() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {
            if (begin == end) return false;
            if (m_(*begin)) {
                Node<C>::onMatch(begin, begin+1);
                ++begin;
                return true;
            }
            return false;
        }

        matchT m_;
    };

    template<typename C>
    LitC<C> operator|(const LitC<C> &a, const LitC<C> &b) {
        return LitC<C>([=](C x) {
            return (a.m_(x) || b.m_(x));
        });
    }

    template<typename C>
    LitC<C> Ch(C ch) {
        return LitC<C>([=](C x) { return ch == x; });
    }

    template<typename C>
    LitC<C> ChSet(std::unordered_set<C> s) {
        return LitC<C>([=](C x) {
            return (s.find(x) != s.end());
        });
    }

    template<typename C>
    LitC<C> ChRange(C a, C b) {
        return LitC<C>([=](C x) {
            if (x >= a && x <= b) {
                return true;
            }
        });
    }

    template<typename C>
    LitC<C> s() {
        return LitC<C>([](C x) {
            return (x == ' ' || x == '\t' || x == '\r' || x == '\n');
        });
    }

    template<typename C>
    LitC<C> w() {
        return LitC<C>([](C x) {
            return ((x >= '0' && x <= '9') ||
                    (x >= 'a' && x <= 'z') ||
                    (x >= 'A' && x <= 'Z') ||
                    x == '_');
        });
    }

    template<typename C>
    LitC<C> d() {
        return LitC<C>([](C x) {
            return (x >= '0' && x <= '9');
        });
    }

    template<typename C>
    LitC<C> any() {
        return LitC<C>([](C x) { return true; });
    }

    template<typename C>
    LitC<C> Not(const LitC<C> &n) {
        return LitC<C>([=](C x) {
            return !(n.m_(x));
        });
    }

    template<typename C>
    class LitS : public Node<C> {
    public:
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using charT = C;

        LitS(strT s)
                : s_(std::move(s)) { }

        virtual ~LitS() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {
            if (s_.size() > (end - begin)) return false;
            auto i = begin;
            auto j = std::begin(s_);

            while (i != end && j != s_.end()) {
                if (*i != *j) return false;
                ++j;
                ++i;
            }
            Node<C>::onMatch(begin, i);
            begin = i;
            return true;
        }

    private:
        strT s_;
    };

    template<typename C>
    LitS<C> Str(typename std::basic_string<C> s) {
        return LitS<C>(std::move(s));
    }

    template<typename C>
    LitS<C> Str(const C *s) {
        return LitS<C>(std::basic_string<C>(s));
    }

    template<typename C, typename L, typename R>
    class Or : public Node<C> {
    public:
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using charT = C;

        Or(L a, R b)
                : a_(std::move(a)), b_(std::move(b)) { }

        virtual ~Or() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {

            auto i = begin;
            if (!a_(i, end)) {
                i = begin;
                a_.reset();
                if (!b_(i, end)) {
                    b_.reset();
                    return false;
                }
            }
            Node<C>::onMatch(begin, i);
            begin = i;
            return true;
        }

        virtual void reset() override {
            Node<C>::reset();
            a_.reset();
            b_.reset();
        }

    private:
        L a_;
        R b_;
    };

    template<typename L, typename R>
    Or<typename L::charT, L, R> operator|(L a, R b) {
        return Or<typename L::charT, L, R>(std::move(a), std::move(b));
    }

    template<typename C, typename L, typename R>
    class And : public Node<C> {
    public:
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using charT = C;

        And(L a, R b)
                : a_(std::move(a)), b_(std::move(b)) { }

        virtual ~And() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {

            auto i = begin;
            if (!(a_(i, end) && b_(i, end))) {
                reset();
                return false;
            }
            Node<C>::onMatch(begin, i);
            begin = i;
            return true;
        }

        virtual void reset() override {
            Node<C>::reset();
            a_.reset();
            b_.reset();
        }

    private:
        L a_;
        R b_;
    };

    template<typename L, typename R>
    And<typename L::charT, L, R> operator+(L a, R b) {
        return And<typename L::charT, L, R>(std::move(a), std::move(b));
    }

    template<typename C, typename N>
    class Repeat : public Node<C> {
    public:
        static const int Infinity = -1;
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using charT = C;

        Repeat(N n, int min, int max = Infinity)
                : n_(std::move(n)), min_(min), max_(max) { }

        virtual ~Repeat() { }

        bool validCount(int count) {
            return count >= min_ && (count <= max_ || max_ == Infinity);
        }

        virtual bool operator()(
                iterT &begin, iterT end) override {

            if (begin == end) {
                if (validCount(0)) {
                    Node<C>::onMatch(begin, begin);
                    return true;
                }
                return false;
            }

            int count = 0;
            auto i = begin;

            while ((count <= max_ || max_ == Infinity) && n_(i, end)) {
                ++count;
            }

            if (validCount(count)) {
                i = begin;
                while (count) {
                    n_(i, end);
                    --count;
                }
                Node<C>::onMatch(begin, i);
                begin = i;
                return true;
            } else {
                // Remove any captures.
                n_.reset();
                return false;
            }
        }

        virtual void reset() override {
            Node<C>::reset();
            n_.reset();
        }

    private:
        N n_;
        int min_;
        int max_;
    };

    template<typename N>
    Repeat<typename N::charT, N> ZoM(N n) {
        return Repeat<typename N::charT, N>(
                std::move(n),
                0,
                Repeat<typename N::charT, N>::Infinity);
    }

    template<typename N>
    Repeat<typename N::charT, N> ZoO(N n) {
        return Repeat<typename N::charT, N>(std::move(n), 0, 1);
    }

    template<typename N>
    Repeat<typename N::charT, N> OoM(N n) {
        return Repeat<typename N::charT, N>(
                std::move(n),
                1,
                Repeat<typename N::charT, N>::Infinity);
    }

    template<typename N>
    N Capture(typename N::strT &s, N n) {
        n.setMatchCallback([&](typename N::strT t) {
            s = std::move(t);
        });
        return n;
    }
}

#endif // URLPARSER_PARSE_H

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

        virtual ~Node() { }

        virtual bool operator()(
                iterT &begin, iterT end) = 0;

        virtual void reset() { }
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
                : a_(std::move(a)), b_(std::move(b)), reentry(false) { }

        virtual ~Or() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {
            auto i = begin;
            if (reentry && begin == beginPrev) {
                if (aPrev) {
                    // Try reentry...
                    if (!a_(i, end)) {
                        aPrev = false;
                    } else {
                        return true;
                    }
                }
                i = begin;
                if (!b_(i, end)) {
                    reset();
                    return false;
                } else {
                    return true;
                }
            }

            if (!a_(i, end)) {
                i = begin;
                if (!b_(i, end)) {
                    reset();
                    return false;
                }
                beginPrev = begin;
                begin = i;
                reentry = true;
                aPrev = false;
                return true;
            }

            reentry = true;
            aPrev = true;
            beginPrev = begin;
            begin = i;
            return true;
        }

        virtual void reset() override {
            reentry = false;
            beginPrev = iterT{};
            a_.reset();
            b_.reset();
        }

    private:
        bool reentry;
        bool aPrev;
        iterT beginPrev;
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
                : a_(std::move(a)), b_(std::move(b)), reentry(false) { }

        virtual ~And() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {

            // Are we reentering this node because of failure later in the match?
            if (reentry && begin == beginPrev) {
                auto i = saved;
                if (b_(i, end)) {
                    begin = i;
                    return true;
                }
            }

            // If we are here, either reentry attempt failed or we are here new.
            auto i = begin;
            while (a_(i, end)) {
                saved = i;
                b_.reset();
                if (b_(i, end)) {
                    beginPrev = begin;
                    begin = i;
                    reentry = true;
                    return true;
                }
                i = begin;
            }

            reset();
            return false;
        }

        virtual void reset() override {
            reentry = false;
            beginPrev = iterT{};
            a_.reset();
            b_.reset();
        }

    private:
        bool reentry;
        iterT beginPrev;
        iterT saved;
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
        static const size_t Infinity = -1;
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using charT = C;

        Repeat(N n, size_t min, size_t max = Infinity)
                : n_(std::move(n)), min_(min), max_(max), reentry(false) { }

        virtual ~Repeat() { }

        bool validCount(size_t count) {
            return count >= min_ && (count <= max_ || max_ == Infinity);
        }

        virtual bool operator()(
                iterT &begin, iterT end) override {

            if (reentry && begin == beginPrev) {
                if (cached.empty()) {
                    reset();
                    return false;
                } else {
                    begin = cached.back();
                    cached.pop_back();
                    return true;
                }
            }

            cached.clear();

            if (begin == end) {
                if (validCount(0)) {
                    reentry = true;
                    beginPrev = begin;
                    return true;
                }
                reset();
                return false;
            }

            auto i = begin;
            size_t count = 0;

            if (validCount(0)) {
                cached.push_back(begin);
            }

            while (n_(i, end)) {
                ++count;
                if (validCount(count)) {
                    cached.push_back(i);
                } else if (count > min_) {
                    // If the count is not valid and is greater than min,
                    // must be greater than max. But we don't use max_ here
                    // because it can have the sentinel value of Infinity.
                    break;
                }
            }

            if (!cached.empty()) {
                beginPrev = begin;
                begin = cached.back();
                cached.pop_back();
                reentry = true;
                return true;
            }
            reset();
            return false;
        }

        virtual void reset() override {
            reentry = false;
            cached.clear();
            beginPrev = iterT{};
            n_.reset();
        }

    private:
        bool reentry;
        iterT beginPrev;
        std::vector<iterT> cached;
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

    template<typename C, typename N>
    class CaptureNode : public Node<C> {
    public:
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using storeT = std::function<void(strT s)>;
        using clearT = std::function<void()>;
        using charT = C;

        CaptureNode(storeT store, clearT clear, N n)
                : store_(std::move(store)), clear_(std::move(clear)), n_(std::move(n)) { }

        virtual ~CaptureNode() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {
            auto i = begin;
            if (n_(i, end)) {
                if (begin == i) {
                    clear_();
                } else {
                    store_(strT(begin, i));
                }
                begin = i;
                return true;
            }
            reset();
            return false;
        }

        virtual void reset() override {
            clear_();
            n_.reset();
        }

        storeT store_;
        clearT clear_;
        N n_;
    };

    template<typename N>
    CaptureNode<typename N::charT, N>
    Capture(typename N::strT &s, N n) {
        return CaptureNode<typename N::charT, N>(
                [&](typename N::strT x) {
                    s = std::move(x);
                },
                [&]() {
                    s.clear();
                },
                std::move(n));
    }
}

#endif // URLPARSER_PARSE_H

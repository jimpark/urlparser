//
//  parse.h
//  parse
//
//  Created by Jim Park on 12/12/15.
//  Copyright © 2015 jimpark. All rights reserved.
//

#ifndef parse_h
#define parse_h

#include <unordered_set>
#include <functional>
#include <string>
#include <iostream>

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

        virtual void onMismatch() { }
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
           if (s.find(x) != s.end()) {
              return true;
           }
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
    LitC<C> NotLitC(const LitC<C> &n) {
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
                : a_(std::move(a)), b_(std::move(b)) { }

        virtual ~Or() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {
           auto i = begin;
           if (!a_(i, end)) {
              i = begin;
              if (!b_(i, end)) {
                 onMismatch();
                 return false;
              }
              begin = i;
              return true;
           }

           begin = i;
           return true;
        }

        virtual void onMismatch() override {
           a_.onMismatch();
           b_.onMismatch();
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
           if (a_(i, end) && b_(i, end)) {
              begin = i;
              return true;
           }
           else {
              onMismatch();
              return false;
           }
        }

        virtual void onMismatch() override {
           a_.onMismatch();
           b_.onMismatch();
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
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using charT = C;

        Repeat(N n, int min, int max = -1)
                : n_(std::move(n)), min_(min), max_(max) { }

        virtual ~Repeat() { }

        bool validCount(int count) {
           return count >= min_ && (count <= max_ || max_ == -1);
        }

        virtual bool operator()(
                iterT &begin, iterT end) override {
           int count = 0;
           if (begin == end) {
              if (validCount(count)) {
                 return true;
              }
              onMismatch();
              return false;
           }

           auto i = begin;
           while (validCount(count + 1) && n_(i, end)) {
              ++count;
           }

           if (validCount(count)) {
              begin = i;
              return true;
           }
           onMismatch();
           return false;
        }

        virtual void onMismatch() override {
           n_.onMismatch();
        }

    private:
        N n_;
        int min_;
        int max_;
    };

    template<typename N>
    Repeat<typename N::charT, N> ZoM(N n) {
       return Repeat<typename N::charT, N>(std::move(n), 0, -1);
    }

    template<typename N>
    Repeat<typename N::charT, N> ZoO(N n) {
       return Repeat<typename N::charT, N>(std::move(n), 0, 1);
    }

    template<typename N>
    Repeat<typename N::charT, N> OoM(N n) {
       return Repeat<typename N::charT, N>(std::move(n), 1, -1);
    }

    template<typename C, typename N>
    class CaptureNode : public Node<C> {
    public:
        using strT = std::basic_string<C>;
        using iterT = typename strT::const_iterator;
        using storeT = std::function<void(strT s)>;
        using charT = C;

        CaptureNode(storeT store, N n)
                : store_(std::move(store)), n_(std::move(n)) { }

        virtual ~CaptureNode() { }

        virtual bool operator()(
                iterT &begin, iterT end) override {
           auto i = begin;
           if (n_(i, end)) {
              store_(strT(begin, i));
              begin = i;
              return true;
           }
           onMismatch();
           return false;
        }

        virtual void onMismatch() override {
           store_(strT());
           n_.onMismatch();
        }

        storeT store_;
        N n_;
    };

    template<typename N>
    CaptureNode<typename N::charT, N>
    Capture(typename N::strT &s, N n) {
       return CaptureNode<typename N::charT, N>(
               [&](typename N::strT x) {
                   s = std::move(x);
               }, std::move(n));
    }

}

#endif /* parse_h */

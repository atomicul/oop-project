#include "phone.h"
#include "phone_cdc.h"
#include <sstream>
#include <utility>

PhoneCDC::PhoneCDC(std::string before_message, std::string after_message,
                   std::vector<Phone *> trace)
    : before_message(std::move(before_message)), after_message(std::move(after_message)),
      _trace(std::move(trace)) {}

PhoneCDC::PhoneCDC(const PhoneCDC &other) = default;

PhoneCDC &PhoneCDC::operator=(PhoneCDC other) {
    swap(*this, other);
    return *this;
}

PhoneCDC::~PhoneCDC() = default;

const std::string &PhoneCDC::beforeMessage() const { return before_message; }

const std::string &PhoneCDC::afterMessage() const { return after_message; }

const std::vector<Phone *> &PhoneCDC::trace() const { return _trace; }

void PhoneCDC::setBeforeMessage(std::string msg) { before_message = std::move(msg); }

void PhoneCDC::setAfterMessage(std::string msg) { after_message = std::move(msg); }

void PhoneCDC::setTrace(std::vector<Phone *> new_trace) { _trace = std::move(new_trace); }

void swap(PhoneCDC &lhs, PhoneCDC &rhs) {
    using std::swap;
    swap(lhs.before_message, rhs.before_message);
    swap(lhs.after_message, rhs.after_message);
    swap(lhs._trace, rhs._trace);
}

std::ostream &operator<<(std::ostream &os, const PhoneCDC &cdc) {
    std::stringstream trace_csv{};
    if (!cdc._trace.empty()) {
        for (std::size_t i = 0; i < cdc._trace.size() - 1; ++i) {
            trace_csv << *cdc._trace[i] << ", ";
        }
        trace_csv << *cdc._trace.back();
    }

    return os << "PhoneCDC { " << "Before=\"" << cdc.before_message << "\", " << "After=\""
              << cdc.after_message << "\", " << "Trace=[" << trace_csv.str() << "] }";
}

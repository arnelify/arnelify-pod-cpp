#ifndef ARNELIFY_ORM_OPTS_HPP
#define ARNELIFY_ORM_OPTS_HPP

#include <iostream>

struct ArnelifyORMOpts final {
  const std::string ORM_DRIVER;
  const std::string ORM_HOST;
  const std::string ORM_NAME;
  const std::string ORM_USER;
  const std::string ORM_PASS;
  const int ORM_PORT;

  ArnelifyORMOpts(const std::string d, const std::string h,
                  const std::string n, const std::string u,
                  const std::string pwd, const int p)
      : ORM_DRIVER(d),
        ORM_HOST(h),
        ORM_NAME(n),
        ORM_USER(u),
        ORM_PASS(pwd),
        ORM_PORT(p) {};
};

#endif
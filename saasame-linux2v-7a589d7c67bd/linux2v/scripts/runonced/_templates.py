import fnmatch


os_families = {
    'RedHat': ['Red', 'CentOS'],
    'Ubuntu': ['Debian', 'Ubuntu']
}
os_templates = {}
for tpl in os_templates:
    os_templates[tpl]['Red'] = os_templates[tpl]['CentOS']


def get_template(setting, os_family, os_version, none_if_no_matched=False):
    os_versions = \
        os_templates[setting].get(os_family) \
        or os_templates[setting].get('*') or {}

    for version in reversed(sorted(os_versions)):
        if fnmatch.fnmatch(os_version, version):
            return os_versions[version]

    if none_if_no_matched:
        return None

    msg = "Given os_version {} and os_family {} not found in templates." \
        .format(os_version, os_family)
    raise Exception(msg)

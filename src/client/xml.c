#include "xml.h"

char *XmlElementGetAttributes(const char *element) {
    size_t len;
    const char *p = element, *e, *n, *i;
    char *r, *s;

    if (!(p = strchr(p, '<')) || !(e = strchr(p, '>'))) /* Check the element closes itself */
        return NULL;

    if ((n = strchr(&p[1], '<')) && n < e) /* Check no new element is being started within the attribute section */
        return NULL;

    i = &p[1];
    while (*i == ' ')
        ++i; /* Jump over any leading whitespace before the Xml element name */

    if (!(i = strchr(i, ' ')) || i > e) /* Find the end of the Xml element name */
        return NULL;  /* if there is no spaces between i and e then attributes are impossible */

    ++i, s = (char *) i, len = 0;
    while (i < e) /* Count the length of the substring */
        ++i, ++len;

    if (!(r = malloc(len + 1)))
        return NULL;

    strncpy(r, s, len), r[len] = '\0';
    return r;
}

char *XmlExtractElement(const char *xml, const char *element) {
    char *p = XmlFindElement(xml, element), *e;

    if (!p || !(e = strchr(p, '>')))
        return NULL;
    else {
        size_t len = 0;
        char *i = p;

        while (i < e)
            ++i, ++len;

        if (len && (i = malloc(len + 2))) {
            ++len, strncpy(i, p, len), i[len] = '\0';
            return i;
        }
    }

    return NULL;
}

char *XmlExtractAttribute(const char *element, const char *attribute) {
    size_t len = strlen(attribute);
    char *a, *t, *r;

    if ((a = XmlElementGetAttributes(element)))
        t = platformStringFindWord(a, attribute);
    else
        return NULL;

    if (t) {
        char *v = XmlExtractAttributeValue(t), qc;
        if (v) {
            char *eq = strchr(t, '='), *singleQuote = strchr(eq, '\''), *doubleQuote = strchr(eq, '"');
            len += strlen(v);

            if (singleQuote && doubleQuote)
                qc = singleQuote < doubleQuote ? '\'' : '"';
            else if (singleQuote)
                qc = '\'';
            else if (doubleQuote)
                qc = '"';
            else
                qc = '\0';

            ++len;
            if (qc != '\0')
                len += 2;
        } else
            qc = '\0';

        if ((r = malloc(len + 1))) {
            size_t alen = strlen(attribute);
            char *i;

            if ((i = malloc(alen + 1))) {
                strncpy(i, t, alen), i[alen] = '\0';

                if (v)
                    sprintf(r, "%s=%c%s%c", i, qc, v, qc);
                else
                    strcpy(r, i);

                free(i);
            } else
                free(r), r = NULL;
        }

        free(v), free(a);
        return r;
    }

    free(a);
    return NULL;
}

char *XmlFindElement(const char *xml, const char *element) {
    const char *p = xml;

    while (p && (p = strchr(p, '<'))) {
        char *next = strchr(&p[1], '<'), *end = strchr(&p[1], '>');
        if (end && end < next) {
            const char *sw = p;
            size_t i, len;

            while ((*sw == ' ' || *sw == '<') && *sw != '\0')
                ++sw;

            if (*sw == '\0' || strlen(sw) < (len = strlen(element))) {
                p = next;
                continue;
            }

            for (i = 0; i < len; ++i) {
                if (toupper(element[i]) != toupper(sw[i]))
                    break;
            }

            if (i == len)
                return (char *) p;
        }
        p = next;
    }

    return NULL;
}

char *XmlFindAttribute(const char *element, const char *attribute) {
    char *a, *t;

    if ((a = XmlElementGetAttributes(element)))
        t = platformStringFindWord(a, attribute);
    else
        return NULL;

    if (t) {
        size_t len = 0;
        char *i = a, *f = strstr(element, a);

        while (i < t)
            ++i, ++len;

        free(a), i = (char *) element;
        while (i < f)
            ++i, ++len;

        return (char *) &element[len];
    }

    free(a);
    return NULL;
}

char *XmlExtractAttributeValue(const char *attribute) {
    size_t len = 0;
    char *eq = strchr(attribute, '='), *qp, qc;

    if (!eq)
        return NULL;

    {
        char *singleQuote = strchr(eq, '\''), *doubleQuote = strchr(eq, '"');
        if (singleQuote && doubleQuote) {
            if (singleQuote < doubleQuote)
                qp = singleQuote, qc = '\'';
            else
                qp = doubleQuote, qc = '"';
        } else if (singleQuote)
            qp = singleQuote, qc = '\'';
        else if (doubleQuote)
            qp = doubleQuote, qc = '"';
        else
            qp = NULL, qc = ' ';
    }

    if (qp) {
        char *i = &qp[1];

        while (*i != '\0') {
            if (*i == qc)
                break;
            ++len, ++i;
        }

        if ((i = malloc(len + 1)))
            strncpy(i, &qp[1], len), i[len] = '\0';

        return i;
    } else {
        char *i = &eq[1];

        while (*i != '\0' || *i == qc)
            ++len, ++i;

        if ((i = malloc(len + 1)))
            strncpy(i, &eq[1], len), i[len] = '\0';

        return i;
    }
}

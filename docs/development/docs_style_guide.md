# LiveCode Documentation Style Guide A-Z

## A

### Abbreviations and acronyms

The first time you use an abbreviation or acronym explain it in full
on each page unless it's well known, eg MP3, PNG etc. Then refer to
it by initials.

Your acronym is well known if you believe that 80% of LiveCode users
will understand, and commonly use, the term.

Don't use full stops in abbreviations - GIF, not G.I.F.

Don't use an acronym if you're not going to use it again later in the
text.

### Active voice

Use the active rather than passive voice. This will help us write
concise, clear documentation.

### Addressing the user

Address the user as 'you' where possible. Documentation often makes a
direct appeal to LiveCode users to take action, eg 'Deploy your stack
to your phone' or 'Store your content in a database'.

### Americanisms

LiveCode originated as a piece of software designed and developed in
the USA.  However, it is now developed and maintained in Scotland, UK.
Please:

* use American English spelling ('color', not 'colour')

* use British English grammar ('really good', not 'real good')

### Ampersand

Use 'and' rather than an '&', unless it's part of syntax.

## B

### Billions

Don't use 'billion'.  There are two distinct definitions of 'billion'
which differ by a factor of a thousand.

If possible, use scientific notation, or a smaller number.

See also 'Millions'.

### Brackets

Use (round brackets), not [square brackets] in documentation text.

Don't use round brackets to refer to something that could either be
singular or plural, eg 'Copy the object(s) to a new stack.'

Always use the plural instead as this will cover each possibility, eg
'Copy the objects to a new stack.'

### Bullets and steps

#### Bullet points

You can use bullet points to make text easier to read. Make sure that:

* you always use a lead-in line
* the bullets make sense running on from the lead-in line
* you use lower case at the start of the bullet
* you don't use more than one sentence per bullet point - use commas,
  dashes or semicolons to expand on an item
* you don't put 'or', 'and' after the bullets
* if you add links they appear within the text and not as the whole
  bullet
* there is no full stop after the last bullet point

#### Steps

Use numbered steps instead of bullet points to guide a user through a
process. You don't need a lead-in line and you can use links and
downloads (with appropriate Markdown) in steps. Each step ends in a
full stop because each step should be a complete sentence.

## C

### Capitalisation

DON'T USE BLOCK CAPITALS FOR LARGE AMOUNTS OF TEXT AS IT'S QUITE HARD
TO READ.

### Contractions

Use contractions, eg 'they've', 'we'll'. Avoid using 'should've',
'could've', 'would've' etc - these are hard to read.

### Dates

Use upper case for months, eg 'January', 'February'.

Don't use a comma between the month and year, eg '14 June 2012'

When space is an issue, eg tables, titles etc, you can use truncated
months, eg 'Jan, Feb, Mar, Aug, Sept, Oct, Nov, Dec'.

Use 'to' in date ranges - not hyphens, en rules or em dashes. For
example:

* 'Monday to Friday, 9am to 5pm' (put different days on a new line,
  don't separate with a comma etc)

* 10 November to 21 December

Don't use 'quarter' for dates; use the months, for example: 'Jan to
Mar 2013'.

## E

### eg, etc and ie

Don't use full stops after or between these notations.

You can use the long form ('for example' instead of 'eg',
'specifically' instead of 'ie' etc) if you like. Some people are not
familiar with abbreviations such as eg, so consider your audience
before abbreviating.

### email

One word.

### Email addresses

Write email addresses in full, in lower case and as active links.
Don't include any other words as part of the link. Don't label email
addresses ie don't write 'Email: name@example.org'.

### etc

See 'eg, etc and ie'.

## F

### FAQs (frequently asked questions)

Don't use FAQs in LiveCode documentation. If you write content by
starting with user needs, you won't need to use FAQs.

## H

### homepage

Lower case.

### Hyphenation

Hyphenate:

* 're-' words starting with 'e', eg 're-evaluate'
* co-ordinate
* co-operate

Don't hyphenate:

* reuse
* reinvent
* reorder
* reopen
* email

Don't use a hyphen unless it's confusing without it. For example, a
little used-car is different from a little-used car.

Use 'to' for time and date ranges, not hyphens.

## I

### ie

See 'eg, etc and ie'

### internet

Lower case

### Italics

Don't use italics. Use 'single quotation marks' if referring to a
document, book, or standard.

## L

### Links

Front-load your link text with the relevant terms and make them active
and specific.

### Lists

Lists should be bulleted to make them easier to read. See 'bullets
and steps'.

Very long lists can be written as a paragraph with a lead-in sentence
if it looks better. Eg 'The following countries are in the EU: Spain,
France, Italy...'

## M

### Mathematical content

Use a minus sign for negative numbers: '−6'

Ratios have no space either side of the colon: '5:12'

One space each side of symbols: +, −, ×, ÷ and =, eg '2 + 2 = 4'

Use the minus sign for subtraction. Use the correct symbol for the
multiplication sign (−×) not the letter x.

Write out and hyphenate fractions: two-thirds, three-quarters

Write out decimal fractions as numerals. Use the same number format
for a sequence: 0.75 and 0.45

### Measurements

Use SI units (not Imperial units).

Use numerals and spell out measurements at first mention.

Don't use a space between the numeral and abbreviated measurement:
'3,500kg' not '3,500 kg'.

Abbreviating kilograms to kg is fine.

If the measurement is more than one word, eg 'kilometres per hour',
then spell it out the first time it is used with the abbreviation.
From then on, abbreviate. If it is only mentioned once, don't
abbreviate.

Use Celsius for temperature, eg 37℃.

### Metaphors

See 'words to avoid'.

### Millions

Always use million in money, eg '£138 million'. Use millions in
phrases, eg 'millions of people'.

However, don't use '£0.xx million' for amounts less than £1 million.

See also 'Billions'.

### Money

Use the £ and $ symbols: '£75', '$90'.

Don't use decimals unless pence are included. For example, use
'£75.50' but not '$75.00'.

Write out 'pence' and 'cents' in full eg 'the user is charged 4 cents
per MB of transfer'.

Currencies are lower case.

### Months

See 'Dates'.

## N

### Numbers

Use 'one' unless you're talking about a step, a point in a list, or another situation where using the numeral makes more sense. For example, like:

> in point 1 of the design instructions'

or this:

> You'll see 5 examples of good LiveCode design practice
>
> There will be:
>
> * 4 examples from mobile app development
> * 1 example from desktop app development

Write all other numbers in numerals (including 2 to 9) except where
it's part of a common expression and it would look strange, eg 'one or
two of them'. Use common sense!

If a number starts a sentence, write it out in full ('Thirty-four
widgets are included in this package') except where it starts a title
or subheading.

For numerals over 999, insert a comma for clarity. 'It was over 9,000'.

Spell out common fractions, such as one-half.

Use a % sign for percentages, eg 50%.

Use a 0 where there's no digit before the decimal point.

Use '500 to 900' and not '500–900' (except in tables).

Use MB not kB for anything over 1MB, eg 4MB not 4096kB. For under
1MB, use kB, eg 569kB not 0.55MB.

Keep it as accurate as possible, up to 2 decimal places. For example:
'4.03MB'.

When using units of bits (b) or bytes (B), SI prefixes should be
factors of 1024.  For all other units, SI prefixes should be factors
of 1000 (as normal).

#### Ordinal numbers

Spell out first to ninth. After that, use 10th etc.

In tables, use numerals throughout.

## O

### online

One word.

### online services

Lower case unless part of a proper noun.

## P

### Per cent

Use per cent not percent. Percentage is 1 word. Always use % with a
number.

### plain English

All content in LiveCode documentation should be written in plain
English. You should also make sure you use language your audience
will understand.

See also 'Americanisms'.

## Q

### Quotes and speech marks

In long passages of speech, open quotes for every new paragraph, but
close quotes only at the end of the final paragraph.

#### Single quotes

Use single quotes:

* in headlines
* for unusual terms
* when referring to words or publications, for example: 'Download the
  'LiveCode Builder Reference Manual' (PDF, 360KB)'

#### Double quotes

Use double quotes in body text for direct quotations.

#### Block quotes

Use the block quote Markdown for quotes longer than a few sentences.

## R

### References

References should be easy to understand by anyone, not just
specialists.

They should follow the style guide. When writing a reference:

* don't use italics
* use single quote marks around titles
* write out abbreviations in full, for example 'page' not 'p', and
  'IEEE Transactions on Computers' not 'IEEE Trans. Comp.'
* use plain English, for example use 'and others' not 'et al'
* don't use full stops after initials or at the end of the reference

If the reference is available online, make the title a link and
include the date you accessed the online version.

## S

### Scrum

Upper case when referring to the framework and method for developing
products, otherwise use lower case.

### seasons

spring, summer, autumn and winter are lowercase.

### Sentence length

Don't use long sentences. Check any sentences with more than 25 words
to see if you can split them to make them clearer.

### spaces

Use only one space after a full stop, not 2.

### Speech marks

See 'Quotes and speech marks'.

### steps

See 'Bullets and steps'.

### Summaries

Summaries should:

* be 140 characters or less
* end with a full stop
* not repeat the title or body text
* be clear and specific

### Synonyms

Avoid using synonyms when referring to LiveCode syntax. Always use the 
canonical form specified in dictionary entries. For example, use 'field'
not 'fld', 'backgroundColor' not 'backColor' or 'secondColor'. 

## T

### technical terms

Where you need to use technical terms, you can. They're not
jargon. You just need to explain what they mean the first time you use
them.

### Temperature

Use Celsius, e.g. 37℃.

### Times

* use 'to' in time ranges, not hyphens, en rules or em dashes. For example, '10am to 11am' not '10–11am'.
* 5:30pm (not 1730hrs)
* midnight, not 00:00
* midday, not 12 noon, noon or 12pm
* 6 hours 30 minutes

### Titles

Titles should:

* be 65 characters or less
* be unique, clear and descriptive
* be front-loaded and optimised for search
* use a colon to break up longer titles
* not contain dashes or slashes
* not have a full stop at the end
* not use acronyms unless they are well-known, eg DVD

## W

### webpage

One word, lower case.

### Wi-Fi

Upper case and hyphenated (trade mark).

### Word document

Upper case, because it's a brand name.

### Words to avoid

Plain English is mandatory for all LiveCode documentation, so please
avoid using these words:

* deliver (pizzas, post and services are delivered, not abstract
  concepts like 'improvements' or 'priorities')
* facilitate (instead, say something specific about how you're
  helping)
* focusing
* impact (don't use this as a synonym for 'have an effect on' or
  'influence')
* initiate
* key (unless it unlocks something. A subject/thing isn't 'key' -
  it's probably 'important')
* monolithic
* progress (as a verb - what are you actually doing?)
* promote (unless you're talking about an ad campaign or some other
  marketing promotion)
* robust
* streamline
* utilise

Avoid using metaphors - they don't say what you actually mean and lead
to slower comprehension of your content. For example:

* going forward (it's unlikely that we are giving travel directions)
* in order to (superfluous - don't use it)

With all of these words you can generally replace them by breaking the
term into what you're actually doing. Be open and specific.

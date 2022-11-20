
from typing import List, Self


class BitFlags:
    """
    Class to handle bit flags
    """

    def __init__(self, flags: List[str] = []):
        """
        :param data: Data to parse
        :param flags: List of flags
        """

        self.ascii_flags = list(map(chr, range(97, 123))) + list(map(chr, range(65, 91)))
        self.bits_set = []

    def parse(self, data: str, offset: int = 0) -> Self:
        self.bits_set = []
        self.append(data, offset)

    def append(self, data: str, offset: int = 0):
        """
        Appends a flag to the list
        :param flag: The flag to append
        :return: None
        """
        if data.lstrip('-').isdigit():
            int_flag = int(data.lstrip('-'))
            if data.startswith('-'):
                int_flag |= 1 << 32

            int_flag = int(data)
            self.bits_set = [i for i in range(32) if int_flag & (1 << i + offset)]

        else:
            self.bits_set = [self.ascii_flags.index(flag) for flag in [*data]]

        self.flags = flags
        return self

    def as_ascii(self):
        """
        :return: The flags as ascii
        """
        return ''.join([self.ascii_flags[i] for i in self.bits_set])

    def __str__(self):
        """
        :return: String representation of the flags
        """
        result = []
        for i in self.bits_set:
            if i < len(self.flags):
                result.append(self.flags[i])
            else:
                result.append(f"UNKNOWN({i})")
        return ', '.join(result)

    def __repr__(self):
        """
        :return: String representation of the flags
        """
        return self.__str__()


if __name__ == '__main__':
    # Test the class
    flags = [f'flag{i}' for i in range(32)]

    print("Testing...")

    bf = BitFlags(flags)
    # Integers
    assert str(bf.parse("0")) == ''
    assert str(bf.parse("1")) == 'flag0'
    assert str(bf.parse("2")) == 'flag1'
    assert str(bf.parse("3")) == 'flag0, flag1'
    assert str(bf.parse("4")) == 'flag2'
    assert str(bf.parse("5")) == 'flag0, flag2'
    assert str(bf.parse("6")) == 'flag1, flag2'
    assert bf.parse("-2147358694").as_ascii() == "bdelnopqF"

    # ASCII

    assert str(bf.parse('')) == ''
    assert str(bf.parse("a")) == 'flag0'
    assert (str(bf.parse("bdelnopqF")) ==
            'flag1, flag3, flag4, flag11, flag13, flag14, flag15, flag16, flag31')
    print("All tests passed!")

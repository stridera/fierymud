from typing import List, Self


class BitFlags:
    """
    Class to handle bit flags
    """

    def __init__(self, flags: List[str] = None):
        """
        :param data: Data to parse
        :param flags: List of flags
        """

        self.ascii_flags = list(map(chr, range(97, 123))) + \
            list(map(chr, range(65, 91)))
        self.bits_set = []
        self.flags = flags

    def reset(self) -> Self:
        self.bits_set = []
        return self

    def parse(self, data: str, offset: int = 0) -> Self:
        self.bits_set = []
        self.set_flags(data, offset)
        return self

    def set_flags(self, data: str, offset: int = 0) -> Self:
        """
        Sets the flags
        :param data: The data to parse
        :return: None
        """
        if data.lstrip('-').isdigit():  # Integer Flags
            int_flag = int(data.lstrip('-'))
            if data.startswith('-'):
                int_flag |= 1 << 32

            int_flag = int(data)
            self.bits_set += [
                i + offset for i in range(32) if int_flag & (1 << i)
            ]
        else:  # ASCII Flags
            self.bits_set += [
                self.ascii_flags.index(flag) + offset for flag in [*data]
            ]

        self.bits_set.sort()
        print(f"{self.flags[0]} - Data: {data}, Flags: {self.bits_set}")
        return self

    def as_ascii(self):
        """
        :return: The flags as ascii
        """
        resp = ''.join([self.ascii_flags[i] for i in self.bits_set])
        return resp if resp else '0'

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

    def to_json(self):
        return str(self)


if __name__ == '__main__':
    # Test the class
    flags = [f'flag{i}' for i in range(32)]

    print("Testing...")

    bf = BitFlags(flags)
    # Integers
    assert str(bf.parse('0')) == '', bf.parse('0')
    assert str(bf.parse('1')) == 'flag0', bf.parse('1')
    assert str(bf.parse('2')) == 'flag1', bf.parse('2')
    assert str(bf.parse('3')) == 'flag0, flag1', bf.parse('3')
    assert str(bf.parse('4')) == 'flag2', bf.parse('4')
    assert str(bf.parse('5')) == 'flag0, flag2', bf.parse('5')
    assert str(bf.parse('6')) == 'flag1, flag2', bf.parse('6')
    assert bf.parse('-2147358694').as_ascii() == 'bdelnopqF', bf.parse(
        '-2147358694').as_ascii()

    # ASCII
    assert str(bf.parse('')) == '', bf.parse('0')
    assert str(bf.parse('a')) == 'flag0', bf.parse('a')
    assert (str(
        bf.parse('bdelnopqF')
    ) == 'flag1, flag3, flag4, flag11, flag13, flag14, flag15, flag16, flag31'
            ), bf.parse('bdelnopqF')

    # Append
    assert str(bf.parse('2').set_flags('1')) == 'flag0, flag1', bf.parse(
        '2').set_flags('1')

    print('All tests passed!')
